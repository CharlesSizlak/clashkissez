#include <stdlib.h>
#include <mongoc/mongoc.h>
#include "contexts.h"
#include "database.h"
#include "chess.h"
#include "vector.h"
#include "hash.h"
#include "security.h"

static void game_invite_2(request_ctx_t *ctx, bson_t **results);
static void game_invite_3(request_ctx_t *ctx, bson_t **results);
static void game_invite_4(request_ctx_t *ctx, bson_t **results);

void game_invite_handler(loop_t *loop, int fd, request_ctx_t *ctx) {
    uint8_t *token;
    if (!GameInviteRequest_get_session_token(ctx->message, &token)) {
        queue_error(ctx, INVALID_PACKET_ERROR);
        GameInviteRequest_free(ctx->message);
        free(ctx);
        return;
    }
    char *str_token = session_token_to_str(token);
    bson_oid_t *oid = str_hash_get(server_ctx.session_tokens, str_token);
    if (oid == NULL) {
        queue_error(ctx, INVALID_SESSION_TOKEN);
        GameInviteRequest_free(ctx->message);
        free(ctx);
        return;
    }
    bson_t *query = bson_new();
    BSON_APPEND_OID(query, "_id", oid);
    database_query(ctx, USERS, query, (database_callback_f)game_invite_2);
}

static void game_invite_2(request_ctx_t *ctx, bson_t **results) {
    // Get our username from the document
    if (results == NULL) {
        queue_error(ctx, DATABASE_ERROR);
        GameInviteRequest_free(ctx->message);
        free(ctx);
        return;
    }
    if (results[0] == NULL) {
        queue_error(ctx, USER_DOESNT_EXIST_ERROR);
        GameInviteRequest_free(ctx->message);
        results_free(results);
        free(ctx);
        return;
    }
    ctx->user_document = results[0];
    // Get the other player's document
    char *username;
    GameInviteRequest_get_username(ctx->message, &username);
    bson_t *query = bson_new();
    BSON_APPEND_UTF8(query, "username", username);
    database_query(ctx, USERS, query, (database_callback_f)game_invite_3);
}

static void game_invite_3(request_ctx_t *ctx, bson_t **results) {
    if (results == NULL) {
        queue_error(ctx, DATABASE_ERROR);
        GameInviteRequest_free(ctx->message);
        free(ctx);
        return;
    }
    if (results[0] == NULL) {
        queue_error(ctx, USER_DOESNT_EXIST_ERROR);
        GameInviteRequest_free(ctx->message);
        results_free(results);
        free(ctx);
        return;
    }
    /*
    example of pulling out a field for later
    if (bson_has_field(results[0], ""))
    {
        bson_iter_t iterator;
        bson_iter_init(&iterator, results[0]);
        bson_iter_find(&iterator, "");
        const bson_value_t *value = bson_iter_value(&iterator);
        memcpy(salt, value->value.v_binary.data, SALT_SIZE);
    }
    */
    // Check the single results OID against the SID in our server_ctx.yadayada
    /*
    bson_t *insert = bson_new();
    bson_t *array = bson_new();
    BSON_APPEND_UTF8(insert, "username", username);
    BSON_APPEND_UTF8(insert, "password", password);
    BSON_APPEND_ARRAY(insert, "games", array);
    bson_oid_init(&ctx->oid, NULL);
    BSON_APPEND_OID(insert, "_id", &ctx->oid);
    */
    bson_t *insert = bson_new();
    bson_iter_t iterator;
    bson_iter_init(&iterator, ctx->user_document);
    bson_iter_find(&iterator, "username");
    const bson_value_t *value = bson_iter_value(&iterator);
    char *inviting_player = value->value.v_utf8.str;
    char *invited_player;
    uint32_t time_control_sender;
    uint32_t time_increment_sender;
    uint32_t time_control_receiver;
    uint32_t time_increment_receiver;
    Color_e color;
    GameInviteRequest_get_username(ctx->message, &invited_player);
    GameInviteRequest_get_time_control_sender(ctx->message, &time_control_sender);
    GameInviteRequest_get_time_increment_sender(ctx->message, &time_increment_sender);
    GameInviteRequest_get_time_control_receiver(ctx->message, &time_control_receiver);
    GameInviteRequest_get_time_increment_receiver(ctx->message, &time_increment_receiver);
    GameInviteRequest_get_color(ctx->message, &color);
    
    BSON_APPEND_BOOL(insert, "pending", true);
    bson_oid_init(&ctx->oid, NULL);
    BSON_APPEND_OID(insert, "_id", &ctx->oid);
    BSON_APPEND_UTF8(insert, "inviting player", inviting_player);
    BSON_APPEND_UTF8(insert, "invited player", invited_player);
    BSON_APPEND_INT64(insert, "time_control_sender", time_control_sender);
    BSON_APPEND_INT64(insert, "time_increment_sender", time_increment_sender);
    BSON_APPEND_INT64(insert, "time_control_receiver", time_control_receiver);
    BSON_APPEND_INT64(insert, "time_increment_receiver", time_increment_receiver);
    BSON_APPEND_INT32(insert, "color", color);

    database_insert(ctx, GAMES, insert, (database_callback_f)game_invite_4);

    char *sid = get_sid(results[0]);
    int_vector_t *vector = str_hash_get(server_ctx.game_invite_subscriptions, sid);
    if (vector == NULL) {
        vector = int_vector_new(1);
        str_hash_add(server_ctx.game_invite_subscriptions, sid, vector);
    }
    char *notif_sid = get_sid(insert);
    GameInviteNotification_t *notif = GameInviteNotification_new();
    GameInviteNotification_set_game_invite_id(notif, notif_sid);
    GameInviteNotification_set_username(notif, inviting_player);
    GameInviteNotification_set_time_control_sender(notif, time_control_sender);
    GameInviteNotification_set_time_increment_sender(notif, time_increment_sender);
    GameInviteNotification_set_time_control_receiver(notif, time_control_receiver);
    GameInviteNotification_set_time_increment_receiver(notif, time_increment_receiver);
    GameInviteNotification_set_color(notif, color);
    uint8_t *buffer;
    size_t buffer_size;
    GameInviteNotification_serialize(notif, &buffer, &buffer_size);
    GameInviteNotification_free(notif);
    // Iterate over vector, send notfication to each subscribed fd
    for (size_t i = 0; i < vector->count; i++) {
        // Take out the fd's connection context if it exists
        // queue up a notification to send for it
        connection_ctx_t *notify_ctx = hash_get(server_ctx.connection_contexts, vector->values[i]);
        if (notify_ctx == NULL) {
            // TODO continue here
        }
        else {
            connection_ctx_queue_write(notify_ctx, buffer, buffer_size);
        }
    }
}

static void game_invite_4(request_ctx_t *ctx, bson_t **results) {
    (void)ctx;
    (void)results;
}
