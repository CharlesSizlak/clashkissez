#include <stdint.h>
#include <stdlib.h>
#include "contexts.h"
#include "chess.h"
#include "handlers.h"

void game_invite_response_subscribe_handler(loop_t *loop, int fd, request_ctx_t *ctx) {
    uint8_t *token;
    if (!GameInviteResponseSubscribe_get_session_token(ctx->message, &token)) {
        queue_error(ctx, INVALID_PACKET_ERROR);
        goto ERROR;
    }
    if (!add_subscription(ctx, server_ctx.game_invite_response_subscriptions, token)) {
        goto ERROR;
    }
    // Fall-through
  ERROR:
    GameInviteResponseSubscribe_free(ctx->message);
    free(ctx);
}
