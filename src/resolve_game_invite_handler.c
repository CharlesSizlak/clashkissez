#include <mongoc/mongoc.h>
#include <stdlib.h>
#include "contexts.h"
#include "database.h"
#include "loop.h"
#include "chess.h"
#include "models.h"
#include "hash.h"
#include "security.h"
#include "handler_utilities.h"
#include "vector.h"


void resolve_game_invite_handler(loop_t *loop, int fd, request_ctx_t *ctx) {
    // Validate their session token
    bson_oid_t *oid;
    VALIDATE_SESSION_TOKEN(ResolveGameInviteRequest, oid);

    // Check the accepted_status bool to see if it's been accepted or declined
    bool accepted_status;
    char *game_invite_id;
    if (!ResolveGameInviteRequest_get_accepted_status(ctx->message, &accepted_status) || 
        !ResolveGameInviteRequest_get_game_invite_id(ctx->message, &game_invite_id)) 
    {
        queue_error(ctx, INVALID_PACKET_ERROR);
        ResolveGameInviteRequest_free(ctx->message);
        free(ctx);
        return;
    }
    // Check if the game is in memory or in the database
    game_t *game = str_hash_get(server_ctx.active_games, game_invite_id);
    // Using game_invite_id update the table in the database with the appropriate info
    // If they accept we start a game
    // If they decline I guess we delete the game invite out of the database
    if (accepted_status) {
        
        /*
        What do we need to do to start a game?
        Send out the notifications that a game has begun, get those rolling
        We need to update our database entry to set the game's pending to false
        Then we need to set up timers for the game and get those rolling
        We'll need some kind of data structure to hold the information that pertains
        to the game, such as the move history and how the timers are doing
        */
       
        if (game != NULL) {
            game->pending = false;
            int_vector_t *inviting_vector = str_hash_get(server_ctx.game_invite_response_subscriptions, game->inviting_sid);
            int_vector_t *invited_vector = str_hash_get(server_ctx.game_invite_response_subscriptions, game->invited_sid);
            GameAcceptNotification_t *notif = GameAcceptNotification_new();
            GameAcceptNotification_set_game_id(notif, game->game_id);
            uint8_t *buffer;
            size_t buffer_size;
            GameAcceptNotification_serialize(notif, &buffer, &buffer_size);
            GameAcceptNotification_free(notif);
            if (inviting_vector != NULL) {
                vector_queue_write(inviting_vector, buffer, buffer_size);
            }
            if (invited_vector != NULL) {
                vector_queue_write(invited_vector, buffer, buffer_size);
            }
            free(buffer);
            /* TODO Get a snapshot of the current time
            Add the players time controls to that to make our two timers
            Slap the two timers in using our handy dandy loop functions
            win game */
        }
        else {
            /* TODO This is for games entered into the database.
            Create timers and slap a copy based on wall clock time into the database
            Also, take a bunch of stuff from the game != NULL path and 
            make it so we're sending out the notifications regardless of the path taken
            and then we just set the vectors and game_id in each path */
        }

    }
    else {
        if (game != NULL) {
            int_vector_t *inviting_vector = str_hash_get(server_ctx.game_invite_response_subscriptions, game->inviting_sid);
            int_vector_t *invited_vector = str_hash_get(server_ctx.game_invite_response_subscriptions, game->invited_sid);
            GameDeclineNotification_t *notif = GameDeclineNotification_new();
            GameDeclineNotification_set_game_id(notif, game->game_id);
            uint8_t *buffer;
            size_t buffer_size;
            GameDeclineNotification_serialize(notif, &buffer, &buffer_size);
            GameDeclineNotification_free(notif);
            if (inviting_vector != NULL) {
                vector_queue_write(inviting_vector, buffer, buffer_size);
            }
            if (invited_vector != NULL) {
                vector_queue_write(invited_vector, buffer, buffer_size);
            }
            free(buffer);
            str_hash_remove(server_ctx.active_games, game_invite_id);
            game_free(game);
            // TODO don't repeat all this code, make this bitch efficient with the other code path
        }
        else {
            // TODO worry about this later when we're adding in database games
        }
    }
}
