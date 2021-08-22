#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdbool.h>
#include <assert.h>
#include <pthread.h>
#include <mongoc/mongoc.h>
#include "queue.h"
#include "hash.h"
#include "chess.h"
#include "loop.h"
#include "debug.h"
#include "random.h"

#define APP_NAME    "clashkissez"
#define TOKEN_SIZE  16

typedef enum database_operation_e {
    QUERY,
    UPDATE,
    INSERT,
    DELETE
} database_operation_e;

typedef enum database_collection_e {
    USERS
} database_collection_e;

typedef struct connection_ctx_t {
    loop_t *loop;
    uint32_t total_bytes_read;
    uint32_t read_buffer_capacity;
    uint8_t *read_buffer;
    uint32_t write_buffer_capacity;
    uint8_t *write_buffer;
    uint32_t bytes_to_write;
    int fd;
    bool close_connection;
} connection_ctx_t;

typedef struct request_ctx_t {
    connection_ctx_t *connection_ctx;
    void *message;
    bson_oid_t oid;
} request_ctx_t;

typedef void (*database_callback_f)(request_ctx_t *, void *);

typedef struct database_operation_t {
    database_operation_e operation;
    database_collection_e collection;
    union {
        bson_t *query;
        bson_t *insert;
        bson_t *delete;
    };
    bson_t *update;
    database_callback_f cb;
    request_ctx_t *ctx;
    void *result;
} database_operation_t;

typedef struct server_ctx_t {
    kh_map_t *connection_contexts;
    kh_str_map_t *session_tokens;
    queue_t *database_queue;
} server_ctx_t;

static server_ctx_t server_ctx;

static void read_write_handler(loop_t *loop, event_e event, int fd, connection_ctx_t *connection_ctx);
static void register_request_continue(request_ctx_t *ctx, bson_t **results);
static void register_request_3(request_ctx_t *ctx, bson_t *result);
static void database_query(request_ctx_t *ctx, database_collection_e collection, bson_t *query, database_callback_f cb);
static void database_insert(request_ctx_t *ctx, database_collection_e collection, bson_t *insert, database_callback_f cb);

static uint8_t *generate_session_token(void) {
    uint8_t *session_token = malloc(TOKEN_SIZE);
    rand_get_bytes(session_token, TOKEN_SIZE);
    return session_token;
}

static char *generate_session_token_str(void) {
    uint8_t session_token[TOKEN_SIZE];
    rand_get_bytes(session_token, TOKEN_SIZE);
    char *str = malloc((TOKEN_SIZE * 2) + 1);
    for (size_t i = 0; i < TOKEN_SIZE; i++)
    {
        sprintf(str + (i * 2), "%02x", session_token[i]);
    }
    return str;
}

static char *session_token_to_str(uint8_t *session_token) {
    char *str = malloc((TOKEN_SIZE * 2) + 1);
    for (size_t i = 0; i < TOKEN_SIZE; i++)
    {
        sprintf(str + (i * 2), "%02x", session_token[i]);
    }
    return str;
}

void sigint_handler(loop_t *loop, int signal, void *data) {
    loop->running = false;
}

void ignore_handler(loop_t *loop, int signal, void *data) {
    // Do nothing
}

void queue_write(request_ctx_t *request_ctx, uint8_t *buffer, size_t buffer_size) {
    DEBUG_PRINTF("Queueing up a write");
    connection_ctx_t *ctx = request_ctx->connection_ctx;
    if (ctx->bytes_to_write + buffer_size > ctx->write_buffer_capacity) {
        do {
            ctx->write_buffer_capacity *= 2;
        } while (ctx->bytes_to_write + buffer_size > ctx->write_buffer_capacity);
        ctx->write_buffer = realloc(ctx->write_buffer, ctx->write_buffer_capacity);
    }
    memcpy(ctx->write_buffer + ctx->bytes_to_write, buffer, buffer_size);
    ctx->bytes_to_write += buffer_size;
    loop_add_fd(ctx->loop, ctx->fd, READ_WRITE_EVENT, (fd_callback_f)read_write_handler, ctx);
}

void queue_error(request_ctx_t *ctx, Error_e error) {
    DEBUG_PRINTF("Queueing up an error to send");
    ErrorReply_t *e = ErrorReply_new();
    ErrorReply_set_error(e, error);
    size_t buffer_size;
    uint8_t *buffer;
    ErrorReply_serialize(e, &buffer, &buffer_size);
    ErrorReply_free(e);
    queue_write(ctx, buffer, buffer_size);
    free(buffer);
}

void not_implemented_handler(loop_t *loop, int fd, request_ctx_t *ctx) {
    // Craft a payload, modify the info in connection context as needed
    DEBUG_PRINTF("Received valid packet that isn't yet supported");
    queue_error(ctx, NOT_IMPLEMENTED_ERROR);
    free(ctx);
}

void invalid_packet_handler(loop_t *loop, int fd, request_ctx_t *ctx) {
    DEBUG_PRINTF("Received a packet that isn't supported");
    queue_error(ctx, INVALID_PACKET_ERROR);
    ctx->connection_ctx->close_connection = true;
    free(ctx);
}


// TODO hey we're starting here, got to make register request work then login :)
void register_request_handler(loop_t *loop, int fd, request_ctx_t *ctx) {
    DEBUG_PRINTF("Made it into the handler");
    // Pull out the username, check to see if it already exists in our database
    char *username;
    char *password;
    if (!RegisterRequest_get_username(ctx->message, &username) ||
            !RegisterRequest_get_password(ctx->message, &password)) {
        RegisterRequest_free(ctx->message);
        invalid_packet_handler(loop, fd, ctx);
        return;
    }
    /*
     *  Users
     {
         Does the username already exist in the database
         AKA
         Does the username exactly match an existing entry in the database
         "username": "Sizlak"
     }
     */
    bson_t *query = bson_new();
    BSON_APPEND_UTF8(query, "username", username);
    database_query(ctx, USERS, query, (database_callback_f)register_request_continue);
}

static void register_request_continue(request_ctx_t *ctx, bson_t **results) {
    DEBUG_PRINTF("Made it into continue");
    // Check if there are any results. If there are results, pack up and send a register failed
    // error, then iterate over results and free the documents there.
    if (results[0] != NULL) {
        queue_error(ctx, REGISTER_FAILED_ERROR);
        RegisterRequest_free(ctx->message);
        for (size_t i = 0;; i++)
        {
            bson_t *document = results[i];
            if (document == NULL) {
                break;
            }
            bson_destroy(document);
        }
        free(results);
        free(ctx);
        return;
    }

    char *username;
    char *password;
    RegisterRequest_get_username(ctx->message, &username);
    RegisterRequest_get_password(ctx->message, &password);
    bson_t *insert = bson_new();
    BSON_APPEND_UTF8(insert, "username", username);
    BSON_APPEND_UTF8(insert, "password", password);
    bson_oid_init(&ctx->oid, NULL);
    BSON_APPEND_OID(insert, "_id", &ctx->oid);
    /*
    {
        "username": "Sizlak",
        "password": "1234"
    }
    */
    database_insert(ctx, USERS, insert, (database_callback_f)register_request_3);
}


// TODO rename this, test to see if we succesfully send a session token back
// TODO fix the double free problem
static void register_request_3(request_ctx_t *ctx, bson_t *result) {
    int32_t insertedCount = 0;
    if (bson_has_field(result, "insertedCount"))
    {
        bson_iter_t iterator;
        bson_iter_init(&iterator, result);
        bson_iter_find(&iterator, "insertedCount");
        const bson_value_t *value = bson_iter_value(&iterator);
        insertedCount = value->value.v_int32;
    }
    if (insertedCount != 1) {
        char *username;
        char *password;
        RegisterRequest_get_username(ctx->message, &username);
        RegisterRequest_get_password(ctx->message, &password);
        bson_t *insert = bson_new();
        BSON_APPEND_UTF8(insert, "username", username);
        BSON_APPEND_UTF8(insert, "password", password);
        bson_oid_init(&ctx->oid, NULL);
        BSON_APPEND_OID(insert, "_id", &ctx->oid);
        database_insert(ctx, USERS, insert, (database_callback_f)register_request_3);
        return;
    }
    bson_oid_t *oid = malloc(sizeof(bson_oid_t));
    bson_oid_copy(&ctx->oid, oid);
    uint8_t *session_token = generate_session_token();
    char *str_token = session_token_to_str(session_token);
    str_hash_add(server_ctx.session_tokens, str_token, oid);
    free(str_token);
    RegisterReply_t *r = RegisterReply_new();
    RegisterReply_set_session_token(r, session_token);
    free(session_token);
    size_t buffer_size;
    uint8_t *buffer;
    RegisterReply_serialize(r, &buffer, &buffer_size);
    RegisterReply_free(r);
    queue_write(ctx, buffer, buffer_size);
    free(buffer);
    RegisterRequest_free(ctx->message);
    free(ctx);
}

void message_handler(loop_t *loop, int fd, connection_ctx_t *connection_ctx)  {
    uint8_t *message = connection_ctx->read_buffer + 4;
    uint32_t message_length = be32toh(*(uint32_t*)connection_ctx->read_buffer);

    request_ctx_t *request_ctx = malloc(sizeof(request_ctx_t));
    request_ctx->connection_ctx = connection_ctx;
    request_ctx->message = NULL;

    if (message_length == 0) {
        DEBUG_PRINTF("Received empty message");
        invalid_packet_handler(loop, fd, request_ctx);
        return;
    }
    
    TableType_e table_type = determine_table_type(message, message_length);
    switch (table_type) {
        case TABLE_TYPE_LoginRequest: {
            DEBUG_PRINTF("Login request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_RegisterRequest: {
            request_ctx->message = RegisterRequest_deserialize(message, message_length);
            register_request_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_UserLookupRequest: {
            DEBUG_PRINTF("UserLookup request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_GameInviteRequest: {
            DEBUG_PRINTF("GameInvite request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_GameAcceptRequest: {
            DEBUG_PRINTF("GameAccept request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_FullGameInformationRequest: {
            DEBUG_PRINTF("FullGameInformation request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_GameMoveRequest: {
            DEBUG_PRINTF("GameMove request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_GameLastMoveRequest: {
            DEBUG_PRINTF("GameLastMove request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_GameHeartbeatRequest: {
            DEBUG_PRINTF("GameHeartbeatRequest request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_GameDrawOfferRequest: {
            DEBUG_PRINTF("GameDrawOffer request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_GameDrawOfferResponse: {
            DEBUG_PRINTF("GameDrawOffer response not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_GameResignationRequest: {
            DEBUG_PRINTF("GameResignationRequest request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_FriendRequest: {
            DEBUG_PRINTF("Friend request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_FriendRequestResponse: {
            DEBUG_PRINTF("FriendRequest response not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_FriendRequestStatusRequest: {
            DEBUG_PRINTF("FriendRequestStatus request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_ActiveGameRequest: {
            DEBUG_PRINTF("ActiveGame request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_GameHistoryRequest: {
            DEBUG_PRINTF("GameHistory request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_PastGameFullInformationRequest: {
            DEBUG_PRINTF("PastGameFullInformation request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        default: {
            invalid_packet_handler(loop, fd, request_ctx);
        }
    }

    // Remove bytes from the read buffer
    memmove(
        connection_ctx->read_buffer, 
        connection_ctx->read_buffer + message_length + 4, 
        connection_ctx->total_bytes_read - (message_length + 4)
    );
    connection_ctx->total_bytes_read -= message_length + 4;
}  

void read_handler(loop_t *loop, event_e event, int fd, connection_ctx_t *connection_ctx)
{
    // Check if we've recieved all the bytes for the message
    // If we have not, read the bytes we do have into memory and store it somehow
    // Then we do all our other shit when we have all our bytes to eat
    if (event == ERROR_EVENT) {
        #ifndef NDEBUG
        int       error = 0;
        socklen_t errlen = sizeof(error);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen) == 0)
        {
            DEBUG_PRINTF("error = %s\n", strerror(error));
        }
        #endif
        goto clean_up;
    }
    uint32_t message_length;
    uint32_t bytes_to_read;
    if (connection_ctx->total_bytes_read < 4) {
        bytes_to_read = 4 - connection_ctx->total_bytes_read;
    }
    else {
        message_length = be32toh(*(uint32_t*)connection_ctx->read_buffer);
        bytes_to_read = message_length - (connection_ctx->total_bytes_read - 4);
    }
    ssize_t bytes_read = read(
        fd,
        connection_ctx->read_buffer + connection_ctx->total_bytes_read, 
        bytes_to_read
    );
    if (bytes_read <= 0) {
        goto clean_up;
    }
    connection_ctx->total_bytes_read += bytes_read;
    if (connection_ctx->total_bytes_read < sizeof(uint32_t))
    {
        // Still waiting on the first 4 bytes to give us the message size
        return;
    }
    message_length = be32toh(*(uint32_t*)connection_ctx->read_buffer);
    if (connection_ctx->total_bytes_read < message_length + 4) {
        return;
    }
    DEBUG_PRINTF("About to call message handler, bytes received: %u\n", connection_ctx->total_bytes_read);
    DEBUG_PRINTF("Read buffer: %02x%02x%02x%02x%02x\n",
        connection_ctx->read_buffer[0],
        connection_ctx->read_buffer[1],
        connection_ctx->read_buffer[2],
        connection_ctx->read_buffer[3],
        connection_ctx->read_buffer[4]);
    message_handler(loop, fd, connection_ctx);
    return;
  clean_up:
    DEBUG_PRINTF("Read error encountered on fd %i", fd);
    loop_remove_fd(loop, fd);
    hash_remove(server_ctx.connection_contexts, fd);
    free(connection_ctx->read_buffer);
    free(connection_ctx->write_buffer);
    free(connection_ctx);
    shutdown(fd, SHUT_RDWR);
    close(fd);
}

void write_handler(loop_t *loop, event_e event, int fd, connection_ctx_t *connection_ctx) {
    DEBUG_PRINTF("Entered write handler");
    if (event == ERROR_EVENT) {
        #ifndef NDEBUG
        int       error = 0;
        socklen_t errlen = sizeof(error);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen) == 0)
        {
            DEBUG_PRINTF("error = %s\n", strerror(error));
        }
        #endif
        goto clean_up;
    }
    ssize_t bytes_written = write(fd, connection_ctx->write_buffer, connection_ctx->bytes_to_write);
    if (bytes_written == -1) {
        goto clean_up;
    }
    DEBUG_PRINTF("Wrote %zd", bytes_written);
    connection_ctx->bytes_to_write -= bytes_written;
    if (connection_ctx->bytes_to_write != 0) {
        memmove(
            connection_ctx->write_buffer, 
            connection_ctx->write_buffer + bytes_written,
            connection_ctx->bytes_to_write
        );
    }
    else if (connection_ctx->close_connection) {
        goto close_connection;
    }
    else
    {
        loop_add_fd(loop, fd, READ_EVENT, (fd_callback_f)read_handler, connection_ctx);
    }
    return;
  clean_up:
    DEBUG_PRINTF("Write error encountered on fd %i", fd);
  close_connection:
    loop_remove_fd(loop, fd);
    hash_remove(server_ctx.connection_contexts, fd);
    free(connection_ctx->read_buffer);
    free(connection_ctx->write_buffer);
    free(connection_ctx);
    shutdown(fd, SHUT_RDWR);
    close(fd);
}

static void read_write_handler(loop_t *loop, event_e event, int fd, connection_ctx_t *connection_ctx) {
    if (event == ERROR_EVENT) {
        #ifndef NDEBUG
        int       error = 0;
        socklen_t errlen = sizeof(error);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen) == 0)
        {
            DEBUG_PRINTF("error = %s\n", strerror(error));
        }
        #endif
        goto clean_up;
    }
    else if (event == READ_EVENT) {
        read_handler(loop, event, fd, connection_ctx);
    }
    else if (event == WRITE_EVENT) {
        write_handler(loop, event, fd, connection_ctx);
    }
    return;
  clean_up:
    DEBUG_PRINTF("Error encountered on fd %i", fd);
    loop_remove_fd(loop, fd);
    hash_remove(server_ctx.connection_contexts, fd);
    free(connection_ctx->read_buffer);
    free(connection_ctx->write_buffer);
    free(connection_ctx);
    shutdown(fd, SHUT_RDWR);
    close(fd);
}

void accept_connection_cb(loop_t *loop, event_e event, int fd, void *data)
{
    // sit on the socket accepting new connections
    struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
    int conn_fd = accept(fd, (struct sockaddr*)&client_addr, &client_len);
    if (conn_fd == -1) {
        printf("Failed to accept connection bruv.\n");
        return;
    }
    printf("Connection accepted!\n");
    connection_ctx_t *connection_ctx = malloc(sizeof(connection_ctx_t));
    connection_ctx->total_bytes_read = 0;
    connection_ctx->read_buffer = malloc(4096);
    connection_ctx->read_buffer_capacity = 4096;
    connection_ctx->write_buffer = malloc(4096);
    connection_ctx->write_buffer_capacity = 4096;
    connection_ctx->bytes_to_write = 0;
    connection_ctx->fd = conn_fd;
    connection_ctx->close_connection = false;
    connection_ctx->loop = loop;
    hash_add(server_ctx.connection_contexts, fd, connection_ctx);
    loop_add_fd(loop, conn_fd, READ_EVENT, (fd_callback_f)read_handler, connection_ctx);
    // In the instance we accept a connection, check their message against
    // a table of legal incoming messages

    // In the instance it's illegal, close the socket

    // In the instance we're legal and good to go, spin up a new fd
    // to handle this connection (which we already have) and put that
    // mfer in the event loop so he can handle the ongoing convo
}

void database_result_handler(loop_t *loop, event_e event, int fd, database_operation_t *data)
{
    DEBUG_PRINTF("Running database_result_handler");
    data->cb(data->ctx, data->result);
    free(data);
}

static void database_query(request_ctx_t *ctx, database_collection_e collection, bson_t *query, database_callback_f cb) {
    DEBUG_PRINTF("Enqueueing query");
    database_operation_t *data = malloc(sizeof(database_operation_t));
    data->collection = collection;
    data->query = query;
    data->ctx = ctx;
    data->operation = QUERY;
    data->cb = cb;
    queue_enqueue(server_ctx.database_queue, data);
}

static void database_insert(request_ctx_t *ctx, database_collection_e collection, bson_t *insert, database_callback_f cb) {
    database_operation_t *data = malloc(sizeof(database_operation_t));
    data->collection = collection;
    data->insert = insert;
    data->ctx = ctx;
    data->operation = INSERT;
    data->cb = cb;
    queue_enqueue(server_ctx.database_queue, data);
}

void *database_thread(loop_t *loop)
{
    mongoc_init();
    mongoc_client_t *client = mongoc_client_new("mongodb://localhost:27017");
    mongoc_client_set_appname(client, APP_NAME);
    mongoc_database_t *database = mongoc_client_get_database(client, APP_NAME);
    mongoc_collection_t *user_collection = mongoc_client_get_collection(client, APP_NAME, "users");

    mongoc_cursor_t *cursor = NULL;
    mongoc_collection_t *collection = NULL;
    const bson_t *document = NULL;
    bson_t *result = NULL;
    
    while (loop->running) {
        size_t result_count = 0;
        size_t result_capacity = 32;
        bson_t **results = malloc((result_capacity + 1) * sizeof(bson_t *));
        // Get an operation from the queue
        database_operation_t *data = queue_dequeue(server_ctx.database_queue);
        DEBUG_PRINTF("Dequeued operation from queue");
        // Get the appropriate collection
        switch (data->collection)
        {
            case USERS:
                collection = user_collection;
            break;
            default:
                return NULL;
        }
        // Perform the operation
        switch (data->operation)
        {
        case QUERY:
            DEBUG_PRINTF("Issuing query to database");
            cursor = mongoc_collection_find_with_opts(collection, data->query, NULL, NULL);
            while (mongoc_cursor_next (cursor, &document)) {
                result = bson_copy(document);
                if (result_count + 1 > result_capacity) {
                    result_capacity *= 2;
                    results = realloc(results, (result_capacity + 1) * sizeof(bson_t));
                }
                results[result_count] = result;
                result_count += 1;
            }
            data->result = results;
            results[result_count] = NULL;
            DEBUG_PRINTF("Trigger result handler on query callback");
            loop_trigger_fd(loop, data->ctx->connection_ctx->fd, (fd_callback_f)database_result_handler, data);
            bson_destroy(data->query);
            mongoc_cursor_destroy(cursor);
        break;
        case UPDATE:
            result = bson_new();
            mongoc_collection_update_one (
                collection, 
                data->query, 
                data->update, 
                NULL, 
                result, 
                NULL
            );
            data->result = result;
            loop_trigger_fd(loop, data->ctx->connection_ctx->fd, (fd_callback_f)database_result_handler, data);
            bson_destroy(data->query);
            bson_destroy(data->update);
        break;
        case INSERT:
            result = bson_new();
            mongoc_collection_insert_one(collection, data->insert, NULL, result, NULL);
            data->result = result;
            loop_trigger_fd(loop, data->ctx->connection_ctx->fd, (fd_callback_f)database_result_handler, data);
            bson_destroy(data->insert);
        break;
        case DELETE:
            result = bson_new();
            mongoc_collection_delete_one (
                collection, 
                data->delete, 
                NULL, 
                result, 
                NULL
            );
            data->result = result;
            loop_trigger_fd(loop, data->ctx->connection_ctx->fd, (fd_callback_f)database_result_handler, data);
            bson_destroy(data->delete);
        break;
        default:
            return NULL;
        }
    }
    mongoc_collection_destroy(user_collection);
    mongoc_database_destroy(database);
    mongoc_client_destroy(client);
    mongoc_cleanup();
    return NULL;
}

int main(int argc, char **arg)
{

    // TODO let the user set PORT from a command line later, defaulting to 15873
    uint16_t PORT = 15873;

    // Initialize random
    rand_init();

    // prepare ourselves for recieving stuff by opening a socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        printf("Failed to open a socket, see ya fucker.\n");
        return 1;
    }

#ifndef NDEBUG
    // Prevent waiting for re-bind in debug mode
    int reuse = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);
#endif

    // TODO at some point catch anything killing us that isn't sigkill, so we can clean up our trash

    // bind to the socket
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);
    int err = bind(server_fd,
	    (struct sockaddr*)&server_addr,
	    sizeof(server_addr));
    if (err == -1) {
        printf("Failed to bind socket. Don't ask me how.\n");
        return 1;
    }

    // listen on the socket
    err = listen(server_fd, 32);
    if (err == -1) {
        printf("Failed to listen on bound socket. How even??");
        return 1;
    }

    loop_t loop_storage;
    loop_t *loop = &loop_storage;
    server_ctx.connection_contexts = kh_init_map();
    server_ctx.session_tokens = kh_init_str_map();
    server_ctx.database_queue = queue_new();

    loop_init(loop);
    
    loop_add_fd(loop, server_fd, READ_EVENT, accept_connection_cb, NULL);
    loop_add_signal(loop, SIGINT, sigint_handler, NULL);
    loop_add_signal(loop, SIGUSR1, ignore_handler, NULL);

    // Spin up a thread to handle our database queries
    pthread_t database_thread_id;
    pthread_create(&database_thread_id, NULL, (thread_f)database_thread, loop);

    loop_run(loop);
    loop_fini(loop);

    int fd;
    connection_ctx_t *connection_ctx;
    kh_foreach(server_ctx.connection_contexts, fd, connection_ctx, {
        free(connection_ctx->read_buffer);
        free(connection_ctx->write_buffer);
        free(connection_ctx);
        shutdown(fd, SHUT_RDWR);
        close(fd);
    })
    kh_destroy_map(server_ctx.connection_contexts);

    shutdown(server_fd, SHUT_RDWR);
    close(server_fd);
    rand_cleanup();


    /*

    list of file descriptors we're interested in
    file descriptors ready2go

    in our loop going brr one of our FD's is awake and ready to go
    check the event registered in FD's loop struct, check to see exactly what's going on
    run code ezpz

    if it's a new connection, we go ahead and try accepting that connection
    if we succeed, slap that in our event loop as an interested fd
    play with the ttl of the fd as wanted depending on what we feel like

    if it's an existing connection, we're going to check the incoming packet
    hash the serialized type of the packet and check it against a table of legal messages
    if it's legal, send the appropriate response
    keep the fd open on the instance we need to send a notif
    if it's illegal, close the connection and get rid of the fd from the event loop

    now for the rough part, doing chess things
    
    You know, we never have a moment where we're really doing a lot
    everything is either a response as a result of a packet coming in, or a response
    to our chess engine replying to us

    If it's our chess engine FD, we send out the appropriate notification if
    there's a fd asssociated with that game it's talking about

    TIMERS
    we have a list of games we're currently managing. We need to have ID's
    to pass around to the chess engine FD's to know what the f*!# they're talking about

    questions for ben
    A) can we make this the kernel's problem and have them YEET us a wakey
    in the instance that a timer hits 0

    B) ask ben about writing data into a file in a way that isn't terrible

    C) currently we don't plan to store draw offers anywhere, so 
    if a player misses the notification they just don't know it happened
    boy that seems annoying to solve if we want them to get it

    cursory exploration looks like we can in fact make this the kernel's problem

    timers we actually care about
    A) how long a connection fd lives before closing
    C) the timer associated with each players clock in a match

    
    return 0;
    */
}
