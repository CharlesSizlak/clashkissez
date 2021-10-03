#include <stdint.h>
#include <stdlib.h>
#include "contexts.h"
#include "chess.h"
#include "handlers.h"

void friend_request_accepted_subscribe_handler(loop_t *loop, int fd, request_ctx_t *ctx) {
    uint8_t *token;
    if (!FriendRequestAcceptedSubscribe_get_session_token(ctx->message, &token)) {
        queue_error(ctx, INVALID_PACKET_ERROR);
        goto ERROR;
    }
    if (!add_subscription(ctx, server_ctx.friend_request_accepted_subscriptions, token)) {
        goto ERROR;
    }

    // Fall-through
  ERROR:
    FriendRequestAcceptedSubscribe_free(ctx->message);
    free(ctx);
}
