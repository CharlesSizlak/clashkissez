#ifndef _CONTEXTS_H
#define _CONTEXTS_H

#include <stdint.h>
#include <mongoc/mongoc.h>
#include "loop.h"
#include "chess.h"
#include "vector.h"
#include "models.h"

/**
 * @brief A connection_ctx_t is a struct created when a new connection is opened.
 * It keeps track of data relating to that connection, most importantly it stores
 * a buffer for writing out bytes along the connection and a buffer
 * to store bytes that are sent to us from the other end of the connection.
 * A connection_ctx_t is freed when a connection is terminated from either end.
 */
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
    bson_oid_t *oid;
} connection_ctx_t;

/**
 * @brief A request_ctx_t is created when a new packet detailing a request for the
 * server comes over a connection. It is used primarily to store information so that
 * you can persist some relevant information between callbacks in the
 * server's event loop.
 * A request_ctx_t is freed when the server resolves the request.
 */
typedef struct request_ctx_t {
    connection_ctx_t *connection_ctx;
    void *message;
    bson_oid_t oid;
    bson_t *user_document;
} request_ctx_t;

/**
 * @brief A singular server_ctx_t is created when the server starts up. It is able
 * to be accessed from anywhere on the server, as such it is used to store information
 * that is useful to many connections or is otherwise important globally.
 * A server_ctx_t is freed when the server shuts down.
 */
typedef struct server_ctx_t {
    kh_map_t *connection_contexts;
    kh_str_map_t *session_tokens;
    queue_t *database_queue;
    kh_str_map_t *game_invite_subscriptions;
    kh_str_map_t *game_invite_response_subscriptions;
    kh_str_map_t *game_subscriptions;
    kh_str_map_t *friend_request_subscriptions;
    kh_str_map_t *friend_request_accepted_subscriptions;
    kh_str_map_t *active_games;
    bson_t *board_state_template;
    uint8_t memory_board_state_template[BOARD_SIZE];
} server_ctx_t;

extern server_ctx_t server_ctx;

/**
 * @brief Queue the provided buffer to be written out to the requests originating fd.
 * Wrapper over connection_ctx_queue_write.
 */
void queue_write(request_ctx_t *ctx, uint8_t *buffer, size_t buffer_size);
/**
 * @brief Queue the provided buffer to be written out to connection's fd.
 */
void connection_ctx_queue_write(connection_ctx_t *ctx, uint8_t *buffer, size_t buffer_size);
/**
 * @brief Take the provided error and send out a ErrorReply along the requests originating fd.
 * Wrapped over connection_ctx_queue_error
 */
void queue_error(request_ctx_t *ctx, Error_e error);
/**
 * @brief Take the provided error and write out a ErrorReply along the connection's fd.
 */
void connection_ctx_queue_error(connection_ctx_t *ctx, Error_e error);
/**
 * @brief Queue to write out the provided buffer to each fd in a given vector
 * 
 * @param vector Vector of file descriptors
 */
void vector_queue_write(int_vector_t *vector, uint8_t *buffer, size_t buffer_size);


#endif
