#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdbool.h>
#include <assert.h>
#include "hash.h"
#include "chess.h"
#include "loop.h"
#include "debug.h"

typedef struct connection_ctx_t {
    uint32_t total_bytes_read;
    uint32_t read_byte_capacity;
    uint8_t *read_bytes;
    uint32_t write_byte_capacity;
    uint8_t *write_bytes;
    uint32_t bytes_to_write;
    int fd;
    bool close_connection;
} connection_ctx_t;

typedef struct server_ctx_t {
    kh_map_t *connection_contexts;
} server_ctx_t;

static server_ctx_t server_ctx;

void read_write_handler(loop_t *loop, event_e event, int fd, connection_ctx_t *connection_ctx);

void sigint_handler(loop_t *loop, int signal, void *data) {
    assert(data == NULL);
    loop->running = false;
}

void queue_write(loop_t *loop, connection_ctx_t *ctx, uint8_t *buffer, size_t buffer_size) {
    if (ctx->bytes_to_write + buffer_size > ctx->write_byte_capacity) {
        do {
            ctx->write_byte_capacity *= 2;
        } while (ctx->bytes_to_write + buffer_size > ctx->write_byte_capacity);
        ctx->write_bytes = realloc(ctx->write_bytes, ctx->write_byte_capacity);
    }
    memcpy(ctx->write_bytes + ctx->bytes_to_write, buffer, buffer_size);
    ctx->bytes_to_write += buffer_size;
    loop_add_fd(loop, ctx->fd, READ_WRITE_EVENT, (fd_callback_f)read_write_handler, ctx);
}

void not_implemented_handler(loop_t *loop, int fd, connection_ctx_t *ctx) {
    // Craft a payload, modify the info in connection context as needed
    DEBUG_PRINTF("Received valid packet that isn't yet supported");
    ErrorReply_t *e = ErrorReply_new();
    ErrorReply_set_error(e, NOT_IMPLEMENTED_ERROR);
    size_t buffer_size;
    uint8_t *buffer;
    ErrorReply_serialize(e, &buffer, &buffer_size);
    ErrorReply_free(e);
    queue_write(loop, ctx, buffer, buffer_size);
    free(buffer);
}

void invalid_packet_handler(loop_t *loop, int fd, connection_ctx_t *ctx) {
    DEBUG_PRINTF("Received a packet that isn't supported");
    ErrorReply_t *e = ErrorReply_new();
    ErrorReply_set_error(e, INVALID_PACKET_ERROR);
    size_t buffer_size;
    uint8_t *buffer;
    ErrorReply_serialize(e, &buffer, &buffer_size);
    ErrorReply_free(e);
    queue_write(loop, ctx, buffer, buffer_size);
    free(buffer);
    ctx->close_connection = true;
}

void message_handler(loop_t *loop, int fd, connection_ctx_t *connection_ctx)  {
    uint8_t *message = connection_ctx->read_bytes + 4;
    uint32_t message_length = be32toh(*(uint32_t*)connection_ctx->read_bytes);
    if (message_length == 0) {
        DEBUG_PRINTF("Received empty message");
        invalid_packet_handler(loop, fd, connection_ctx);
        return;
    }
    TableType_e table_type = determine_table_type(message, message_length);
    switch (table_type) {
        case TABLE_TYPE_LoginRequest: {
            DEBUG_PRINTF("Login request not implemented");
            not_implemented_handler(loop, fd, connection_ctx);
        }
        break;
        case TABLE_TYPE_RegisterRequest: {
            DEBUG_PRINTF("Register request not implemented");
            not_implemented_handler(loop, fd, connection_ctx);
        }
        break;
        case TABLE_TYPE_UserLookupRequest: {
            DEBUG_PRINTF("UserLookup request not implemented");
            not_implemented_handler(loop, fd, connection_ctx);
        }
        break;
        case TABLE_TYPE_GameInviteRequest: {
            DEBUG_PRINTF("GameInvite request not implemented");
            not_implemented_handler(loop, fd, connection_ctx);
        }
        break;
        case TABLE_TYPE_GameAcceptRequest: {
            DEBUG_PRINTF("GameAccept request not implemented");
            not_implemented_handler(loop, fd, connection_ctx);
        }
        break;
        case TABLE_TYPE_FullGameInformationRequest: {
            DEBUG_PRINTF("FullGameInformation request not implemented");
            not_implemented_handler(loop, fd, connection_ctx);
        }
        break;
        case TABLE_TYPE_GameMoveRequest: {
            DEBUG_PRINTF("GameMove request not implemented");
            not_implemented_handler(loop, fd, connection_ctx);
        }
        break;
        case TABLE_TYPE_GameLastMoveRequest: {
            DEBUG_PRINTF("GameLastMove request not implemented");
            not_implemented_handler(loop, fd, connection_ctx);
        }
        break;
        case TABLE_TYPE_GameHeartbeatRequest: {
            DEBUG_PRINTF("GameHeartbeatRequest request not implemented");
            not_implemented_handler(loop, fd, connection_ctx);
        }
        break;
        case TABLE_TYPE_GameDrawOfferRequest: {
            DEBUG_PRINTF("GameDrawOffer request not implemented");
            not_implemented_handler(loop, fd, connection_ctx);
        }
        break;
        case TABLE_TYPE_GameDrawOfferResponse: {
            DEBUG_PRINTF("GameDrawOffer response not implemented");
            not_implemented_handler(loop, fd, connection_ctx);
        }
        break;
        case TABLE_TYPE_GameResignationRequest: {
            DEBUG_PRINTF("GameResignationRequest request not implemented");
            not_implemented_handler(loop, fd, connection_ctx);
        }
        break;
        case TABLE_TYPE_FriendRequest: {
            DEBUG_PRINTF("Friend request not implemented");
            not_implemented_handler(loop, fd, connection_ctx);
        }
        break;
        case TABLE_TYPE_FriendRequestResponse: {
            DEBUG_PRINTF("FriendRequest response not implemented");
            not_implemented_handler(loop, fd, connection_ctx);
        }
        break;
        case TABLE_TYPE_FriendRequestStatusRequest: {
            DEBUG_PRINTF("FriendRequestStatus request not implemented");
            not_implemented_handler(loop, fd, connection_ctx);
        }
        break;
        case TABLE_TYPE_ActiveGameRequest: {
            DEBUG_PRINTF("ActiveGame request not implemented");
            not_implemented_handler(loop, fd, connection_ctx);
        }
        break;
        case TABLE_TYPE_GameHistoryRequest: {
            DEBUG_PRINTF("GameHistory request not implemented");
            not_implemented_handler(loop, fd, connection_ctx);
        }
        break;
        case TABLE_TYPE_PastGameFullInformationRequest: {
            DEBUG_PRINTF("PastGameFullInformation request not implemented");
            not_implemented_handler(loop, fd, connection_ctx);
        }
        break;
        default: {
            invalid_packet_handler(loop, fd, connection_ctx);
        }
    }
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
        message_length = be32toh(*(uint32_t*)connection_ctx->read_bytes);
        bytes_to_read = message_length - (connection_ctx->total_bytes_read - 4);
    }
    ssize_t bytes_read = read(
        fd,
        connection_ctx->read_bytes + connection_ctx->total_bytes_read, 
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
    message_length = be32toh(*(uint32_t*)connection_ctx->read_bytes);
    if (connection_ctx->total_bytes_read < message_length + 4) {
        return;
    }
    DEBUG_PRINTF("About to call message handler, bytes received: %u\n", connection_ctx->total_bytes_read);
    DEBUG_PRINTF("Read buffer: %02x%02x%02x%02x%02x\n",
        connection_ctx->read_bytes[0],
        connection_ctx->read_bytes[1],
        connection_ctx->read_bytes[2],
        connection_ctx->read_bytes[3],
        connection_ctx->read_bytes[4]);
    message_handler(loop, fd, connection_ctx);
    return;
  clean_up:
    DEBUG_PRINTF("Read error encountered on fd %i", fd);
    loop_remove_fd(loop, fd);
    hash_remove(server_ctx.connection_contexts, fd);
    free(connection_ctx->read_bytes);
    free(connection_ctx->write_bytes);
    free(connection_ctx);
    shutdown(fd, SHUT_RDWR);
    close(fd);
}

void write_handler(loop_t *loop, event_e event, int fd, connection_ctx_t *connection_ctx) {
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
    ssize_t bytes_written = write(fd, connection_ctx->write_bytes, connection_ctx->bytes_to_write);
    if (bytes_written == -1) {
        goto clean_up;
    }
    connection_ctx->bytes_to_write -= bytes_written;
    if (connection_ctx->bytes_to_write != 0) {
        memmove(
            connection_ctx->write_bytes, 
            connection_ctx->write_bytes + bytes_written,
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
    free(connection_ctx->read_bytes);
    free(connection_ctx->write_bytes);
    free(connection_ctx);
    shutdown(fd, SHUT_RDWR);
    close(fd);
}

void read_write_handler(loop_t *loop, event_e event, int fd, connection_ctx_t *connection_ctx) {
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
    free(connection_ctx->read_bytes);
    free(connection_ctx->write_bytes);
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
    connection_ctx->read_bytes = malloc(4096);
    connection_ctx->read_byte_capacity = 4096;
    connection_ctx->write_bytes = malloc(4096);
    connection_ctx->write_byte_capacity = 4096;
    connection_ctx->bytes_to_write = 0;
    connection_ctx->fd = fd;
    connection_ctx->close_connection = false;
    hash_add(server_ctx.connection_contexts, fd, connection_ctx);
    loop_add_fd(loop, conn_fd, READ_EVENT, (fd_callback_f)read_handler, connection_ctx);
    // In the instance we accept a connection, check their message against
    // a table of legal incoming messages

    // In the instance it's illegal, close the socket

    // In the instance we're legal and good to go, spin up a new fd
    // to handle this connection (which we already have) and put that
    // mfer in the event loop so he can handle the ongoing convo
}


int main(int argc, char **arg)
{
    // TODO let the user set PORT from a command line later, defaulting to 15873
    uint16_t PORT = 15873;


    // prepare ourselves for recieving stuff by opening a socket

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        printf("Failed to open a socket, see ya fucker.\n");
        return 1;
    }
    // bind to the socket
    // TODO at some point catch anything killing us that isn't sigkill, so we can clean up our trash

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

    loop_init(loop);
    
    loop_add_fd(loop, server_fd, READ_EVENT, accept_connection_cb, NULL);
    loop_add_signal(loop, SIGINT, sigint_handler, NULL);

    loop_run(loop);
    loop_fini(loop);

    int fd;
    connection_ctx_t *connection_ctx;
    kh_foreach(server_ctx.connection_contexts, fd, connection_ctx, {
        free(connection_ctx->read_bytes);
        free(connection_ctx->write_bytes);
        free(connection_ctx);
        shutdown(fd, SHUT_RDWR);
        close(fd);
    })
    kh_destroy_map(server_ctx.connection_contexts);

    shutdown(server_fd, SHUT_RDWR);
    close(server_fd);


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
