#ifndef _HANDLERS_H
#define _HANDLERS_H

#include <stdint.h>
#include <stdbool.h>
#include "contexts.h"
#include "hash.h"
#include "loop.h"

/**
 * @brief Adds a user's connection to a vector in a hashtable that will be used to notify
 * the user if a relevant event to their subscription occurs.
 */
bool add_subscription(request_ctx_t *ctx, kh_str_map_t *hashtable, uint8_t *token);

/*
 * @brief All of the following functions are called when a packet of that type comes from a user
 */
void register_request_handler(loop_t *loop, int fd, request_ctx_t *ctx);
void login_request_handler(loop_t *loop, int fd, request_ctx_t *ctx);
void game_invite_handler(loop_t *loop, int fd, request_ctx_t *ctx);
void game_invite_subscribe_handler(loop_t *loop, int fd, request_ctx_t *ctx);
void game_subscribe_handler(loop_t *loop, int fd, request_ctx_t *ctx);
void game_invite_response_subscribe_handler(loop_t *loop, int fd, request_ctx_t *request_ctx);
void friend_request_subscribe_handler(loop_t *loop, int fd, request_ctx_t *ctx);
void friend_request_accepted_subscribe_handler(loop_t *loop, int fd, request_ctx_t *ctx);
void resolve_game_invite_handler(loop_t *loop, int fd, request_ctx_t *ctx);
void not_implemented_handler(loop_t *loop, int fd, request_ctx_t *ctx);
void invalid_packet_handler(loop_t *loop, int fd, request_ctx_t *ctx);

/**
 * @brief Reads incoming bytes from a connection and places them into a buffer.
 * Calls message_handler when all incoming bytes have been read.
 */
void read_handler(loop_t *loop, event_e event, int fd, connection_ctx_t *connection_ctx);

/**
 * @brief Writes out bytes along a connection.
 */
void write_handler(loop_t *loop, event_e event, int fd, connection_ctx_t *connection_ctx);

/**
 * @brief Calls the appropriate read handler or write handler.
 */
void read_write_handler(loop_t *loop, event_e event, int fd, connection_ctx_t *connection_ctx);

#endif
