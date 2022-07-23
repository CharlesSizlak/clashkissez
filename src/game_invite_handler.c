#include <stdlib.h>
#include <mongoc/mongoc.h>
#include "contexts.h"
#include "database.h"
#include "chess.h"
#include "vector.h"
#include "hash.h"
#include "security.h"
#include "models.h"
#include "handler_utilities.h"
#include "debug.h"

#define MS_IN_A_DAY (1000*60*60*24)
#define MS_IN_AN_HOUR (1000*60*60)

static void game_invite_get_user_document(request_ctx_t *ctx, bson_t **results);
static void game_invite_get_invitee_document_and_notify(request_ctx_t *ctx, bson_t **results);
static void game_invite_reply(request_ctx_t *ctx, bson_t *result);

void game_invite_handler(loop_t *loop, int fd, request_ctx_t *ctx) {
    bson_oid_t *oid;
    VALIDATE_SESSION_TOKEN(GameInviteRequest, oid);
    bson_t *query = bson_new();
    BSON_APPEND_OID(query, "_id", oid);
    DEBUG_PRINTF("All right, we got to the first step. Querying database.");
    database_query(ctx, USERS, query, (database_callback_f)game_invite_get_user_document);
}

static void game_invite_get_user_document(request_ctx_t *ctx, bson_t **results) {
    // Get our username from the document
    VALIDATE_QUERY_RESULTS(GameInviteRequest, USER_DOESNT_EXIST_ERROR);
    ctx->user_document = results[0];
    free(results);
    // Get the other player's document
    char *username;
    if (!GameInviteRequest_get_username(ctx->message, &username)) 
    {
        queue_error(ctx, INVALID_PACKET_ERROR);
        GameInviteRequest_free(ctx->message);
        free(ctx);
        return;
    }
    bson_t *query = bson_new();
    BSON_APPEND_UTF8(query, "username", username);
    DEBUG_PRINTF("All right, second step. Querying database again");
    database_query(ctx, USERS, query, (database_callback_f)game_invite_get_invitee_document_and_notify);
}

static void store_game_in_memory(
    request_ctx_t *ctx,
    const char *sid,
    const char *inviting_player,
    const char *invited_player,
    const char *inviting_player_sid,
    const char *invited_player_sid,
    uint32_t time_control_sender,
    uint32_t time_increment_sender,
    uint32_t time_control_receiver,
    uint32_t time_increment_receiver,
    Color_e color)
{
    game_t *game = malloc(sizeof(game_t));
    game->game_id = strdup(sid);
    game->invited_player = strdup(invited_player);
    game->inviting_player = strdup(inviting_player);
    game->inviting_sid = strdup(inviting_player_sid);
    game->invited_sid = strdup(invited_player_sid);
    game->inviting_player_color = color;
    game->pending = true;
    game->time_control_receiver = time_control_receiver;
    game->time_control_sender = time_control_sender;
    game->time_increment_receiver = time_increment_receiver;
    game->time_increment_sender = time_increment_sender;
    memcpy(game->board_state, server_ctx.memory_board_state_template, BOARD_SIZE);
    game->moves_made = 0;
    game->white_short_castle_rights = true;
    game->white_long_castle_rights = true;
    game->black_short_castle_rights = true;
    game->black_long_castle_rights = true;
    str_hash_add(server_ctx.active_games, sid, game);
}

static void game_invite_get_invitee_document_and_notify(request_ctx_t *ctx, bson_t **results) {
    VALIDATE_QUERY_RESULTS(GameInviteRequest, USER_DOESNT_EXIST_ERROR);
    bson_t *invitee = results[0];
    free(results);
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
    char *inviting_sid = get_sid(ctx->user_document);
    char *invited_sid = get_sid(invitee);
    bson_destroy(invitee);

    bson_oid_init(&ctx->oid, NULL);
    BSON_APPEND_OID(insert, "_id", &ctx->oid);
    char *notif_sid = get_sid(insert);

    // Check if either of the time_controls are > 1 day, or if the time_increments are > 1 hour
    bool stored_game_in_memory = false;
    if (time_control_sender >= MS_IN_A_DAY ||
        time_control_receiver >= MS_IN_A_DAY ||
        time_increment_sender >= MS_IN_AN_HOUR ||
        time_increment_receiver >= MS_IN_AN_HOUR) 
    {
        BSON_APPEND_BOOL(insert, "pending", true);
        BSON_APPEND_UTF8(insert, "inviting player", inviting_player);
        BSON_APPEND_UTF8(insert, "inviting player sid", inviting_sid);
        BSON_APPEND_UTF8(insert, "invited player", invited_player);
        BSON_APPEND_UTF8(insert, "invited player sid", invited_sid);
        BSON_APPEND_INT64(insert, "time control sender", time_control_sender);
        BSON_APPEND_INT64(insert, "time increment sender", time_increment_sender);
        BSON_APPEND_INT64(insert, "time control receiver", time_control_receiver);
        BSON_APPEND_INT64(insert, "time increment receiver", time_increment_receiver);
        BSON_APPEND_INT32(insert, "color", color);
        BSON_APPEND_ARRAY(insert, "board state", server_ctx.board_state_template);
        bson_t *move_history = bson_new();
        BSON_APPEND_ARRAY(insert, "move history", move_history);
        bson_free(move_history);
        BSON_APPEND_INT32(insert, "moves made", 0);
        BSON_APPEND_BOOL(insert, "white short castle rights", true);
        BSON_APPEND_BOOL(insert, "white long castle rights", true);
        BSON_APPEND_BOOL(insert, "black short castle rights", true);
        BSON_APPEND_BOOL(insert, "black long castle rights", true);

        database_insert(ctx, GAMES, insert, (database_callback_f)game_invite_reply);
    }
    else {
        DEBUG_PRINTF("We should be storing the game in memory now");
        store_game_in_memory(
            ctx, 
            notif_sid, 
            inviting_player, 
            invited_player,
            inviting_sid,
            invited_sid, 
            time_control_sender,
            time_increment_sender,
            time_control_receiver,
            time_increment_receiver,
            color
        );
        GameInviteReply_t *reply = GameInviteReply_new();
        size_t buffer_size;
        uint8_t *buffer;
        GameInviteReply_serialize(reply, &buffer, &buffer_size);
        GameInviteReply_free(reply);
        queue_write(ctx, buffer, buffer_size);
        free(buffer);
        stored_game_in_memory = true;
    }

    int_vector_t *vector = str_hash_get(server_ctx.game_invite_subscriptions, invited_sid);
    if (vector == NULL) {
        vector = int_vector_new(1);
        str_hash_add(server_ctx.game_invite_subscriptions, invited_sid, vector);
    }
    else {
        free(invited_sid);
    }
    
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
    DEBUG_PRINTF("Sending out notifs");
    // Iterate over vector, send notfication to each subscribed fd
    for (size_t i = 0; i < vector->count; i++) {
        // Take out the fd's connection context if it exists
        // queue up a notification to send for it
        connection_ctx_t *notify_ctx = hash_get(server_ctx.connection_contexts, vector->values[i]);
        if (notify_ctx == NULL) {
            int_vector_remove_index(vector, i);
            i -= 1;
            continue;
        }
        else {
            connection_ctx_queue_write(notify_ctx, buffer, buffer_size);
        }
    }
    free(buffer);
    free(notif_sid);
    free(inviting_sid);
    bson_destroy(ctx->user_document);
    if (stored_game_in_memory) {
        GameInviteRequest_free(ctx->message);
        free(ctx);
    }
}

// TODO Start a first move timer.
static void game_invite_reply(request_ctx_t *ctx, bson_t *result) {
    if (result == NULL) {
        queue_error(ctx, DATABASE_ERROR);
    }
    else {
        bson_destroy(result);
        GameInviteReply_t *reply = GameInviteReply_new();
        size_t buffer_size;
        uint8_t *buffer;
        GameInviteReply_serialize(reply, &buffer, &buffer_size);
        GameInviteReply_free(reply);
        queue_write(ctx, buffer, buffer_size);
        free(buffer);
    }
    GameInviteRequest_free(ctx->message);
    free(ctx);
}
