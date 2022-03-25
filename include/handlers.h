#ifndef _HANDLERS_H
#define _HANDLERS_H

#include <stdint.h>
#include <stdbool.h>
#include "contexts.h"
#include "hash.h"
#include "loop.h"

bool add_subscription(request_ctx_t *ctx, kh_str_map_t *hashtable, uint8_t *token);

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

void read_handler(loop_t *loop, event_e event, int fd, connection_ctx_t *connection_ctx);
void write_handler(loop_t *loop, event_e event, int fd, connection_ctx_t *connection_ctx);
void read_write_handler(loop_t *loop, event_e event, int fd, connection_ctx_t *connection_ctx);

#endif
