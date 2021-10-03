#include <stdint.h>
#include <stdlib.h>
#include "contexts.h"
#include "chess.h"
#include "handlers.h"

void game_invite_subscribe_handler(loop_t *loop, int fd, request_ctx_t *ctx) {
    // We'll need to map oid's to a vector of fd's
    uint8_t *token;
    if (!GameInviteSubscribe_get_session_token(ctx->message, &token)) {
        queue_error(ctx, INVALID_PACKET_ERROR);
        goto ERROR;
    }
    if (!add_subscription(ctx, server_ctx.game_invite_subscriptions, token)) {
        goto ERROR;
    }

    // Fall-through
  ERROR:
    GameInviteSubscribe_free(ctx->message);
    free(ctx);
}
