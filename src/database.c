#include <mongoc/mongoc.h>
#include "database.h"
#include "contexts.h"
#include "security.h"
#include "debug.h"


#define APP_NAME "clashkissez"

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

void results_free(bson_t **results) {
    for (size_t i = 0;; i++)
    {
        bson_t *document = results[i];
        if (document == NULL) {
            break;
        }
        bson_destroy(document);
    }
    free(results);
}

bson_oid_t *get_oid(bson_t *obj) {
    bson_oid_t *oid = malloc(sizeof(bson_oid_t));
    bson_iter_t iterator;
    bson_iter_init(&iterator, obj);
    bson_iter_find(&iterator, "_id");
    const bson_value_t *value = bson_iter_value(&iterator);
    bson_oid_copy(&value->value.v_oid, oid);
    return oid;
}

char *get_sid(bson_t *obj) {
    bson_oid_t *oid = get_oid(obj);
    char *sid = malloc(SID_SIZE);
    bson_oid_to_string(oid, sid);
    free(oid);
    return sid;
}


static void database_result_handler(loop_t *loop, event_e event, int fd, database_operation_t *data)
{
    data->cb(data->ctx, data->result);
    free(data);
}

void database_query(request_ctx_t *ctx, database_collection_e collection, bson_t *query, database_callback_f cb) {
    database_operation_t *data = malloc(sizeof(database_operation_t));
    data->collection = collection;
    data->query = query;
    data->ctx = ctx;
    data->operation = QUERY;
    data->cb = cb;
    queue_enqueue(server_ctx.database_queue, data);
}

void database_insert(request_ctx_t *ctx, database_collection_e collection, bson_t *insert, database_callback_f cb) {
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
    char *connection_string;
    asprintf(
        &connection_string,
        "mongodb://%s:%s@chess-mongo:27017",
        getenv("MONGO_USERNAME"),
        getenv("MONGO_PASSWORD")
    );
    DEBUG_PRINTF("Connection string: %s", connection_string);
    mongoc_client_t *client = mongoc_client_new(connection_string);
    free(connection_string);
    mongoc_client_set_appname(client, APP_NAME);
    mongoc_database_t *database = mongoc_client_get_database(client, APP_NAME);
    mongoc_collection_t *user_collection = mongoc_client_get_collection(client, APP_NAME, "users");
    mongoc_collection_t *game_collection = mongoc_client_get_collection(client, APP_NAME, "games");

    mongoc_cursor_t *cursor = NULL;
    mongoc_collection_t *collection = NULL;
    const bson_t *document = NULL;
    bson_t stack_result;
    bson_t *result = NULL;
    bson_error_t error;
    bool status;
    
    while (loop->running) {
        // Get an operation from the queue
        database_operation_t *data = queue_dequeue(server_ctx.database_queue);
        if (data == NULL)
        {
            break;
        }
        // Get the appropriate collection
        switch (data->collection)
        {
            case USERS:
                collection = user_collection;
            break;
            case GAMES:
                collection = game_collection;
            break;
            default:
                data->result = NULL;
                loop_trigger_fd(loop, data->ctx->connection_ctx->fd, (fd_callback_f)database_result_handler, data);
                continue;
        }
        // Perform the operation
        switch (data->operation)
        {
        case QUERY: {
            size_t result_count = 0;
            size_t result_capacity = 32;
            bson_t **results = malloc((result_capacity + 1) * sizeof(bson_t *));
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
            results[result_count] = NULL;
            if (mongoc_cursor_error(cursor, &error)) {
                DEBUG_PRINTF("Failed to iterate all documents: %s\n", error.message);
                data->result = NULL;
                results_free(results);
            }
            else {
                data->result = results;
            }
            loop_trigger_fd(loop, data->ctx->connection_ctx->fd, (fd_callback_f)database_result_handler, data);
            bson_destroy(data->query);
            mongoc_cursor_destroy(cursor);
        }
        break;
        case UPDATE:
            result = bson_new();
            status = mongoc_collection_update_one(
                collection, 
                data->query, 
                data->update, 
                NULL, 
                result, 
                &error
            );
            if (!status) {
                DEBUG_PRINTF("Failed to update document: %s\n", error.message);
                bson_destroy(result);
                data->result = NULL;
            }
            else {
                data->result = result;
            }
            loop_trigger_fd(loop, data->ctx->connection_ctx->fd, (fd_callback_f)database_result_handler, data);
            bson_destroy(data->query);
            bson_destroy(data->update);
        break;
        case INSERT:
            status = mongoc_collection_insert_one(
                collection, 
                data->insert, 
                NULL, 
                &stack_result, 
                &error
            );
            if (!status) {
                DEBUG_PRINTF("Failed to insert document: %s\n", error.message);
                data->result = NULL;
                bson_destroy(&stack_result);
            }
            else {
                data->result = bson_copy(&stack_result);
                bson_destroy(&stack_result);
            }
            loop_trigger_fd(loop, data->ctx->connection_ctx->fd, (fd_callback_f)database_result_handler, data);
            bson_destroy(data->insert);
        break;
        case DELETE:
            result = bson_new();
            status = mongoc_collection_delete_one (
                collection, 
                data->delete, 
                NULL, 
                result, 
                &error
            );
            if (!status) {
                DEBUG_PRINTF("Failed to delete document: %s\n", error.message);
                bson_destroy(result);
                data->result = NULL;
            }
            else {
                data->result = result;
            }
            loop_trigger_fd(loop, data->ctx->connection_ctx->fd, (fd_callback_f)database_result_handler, data);
            bson_destroy(data->delete);
        break;
        default:
            data->result = NULL;
            loop_trigger_fd(loop, data->ctx->connection_ctx->fd, (fd_callback_f)database_result_handler, data);
            continue;
        }
    }
    mongoc_collection_destroy(user_collection);
    mongoc_collection_destroy(game_collection);
    mongoc_database_destroy(database);
    mongoc_client_destroy(client);
    mongoc_cleanup();
    return NULL;
}
