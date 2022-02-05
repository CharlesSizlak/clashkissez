#ifndef _CONTEXTS_H
#define _CONTEXTS_H

#include <stdint.h>
#include <mongoc/mongoc.h>
#include "loop.h"
#include "chess.h"
#include "vector.h"
#include "models.h"

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

typedef struct request_ctx_t {
    connection_ctx_t *connection_ctx;
    void *message;
    bson_oid_t oid;
    bson_t *user_document;
} request_ctx_t;

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

void queue_write(request_ctx_t *ctx, uint8_t *buffer, size_t buffer_size);
void connection_ctx_queue_write(connection_ctx_t *ctx, uint8_t *buffer, size_t buffer_size);
void queue_error(request_ctx_t *ctx, Error_e error);
void connection_ctx_queue_error(connection_ctx_t *ctx, Error_e error);
void vector_queue_write(int_vector_t *vector, uint8_t *buffer, size_t buffer_size);


#endif
