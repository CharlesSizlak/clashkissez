#include "contexts.h"
#include "database.h"
#include "loop.h"
#include "chess.h"
#include "models.h"
#include "hash.h"
#include "security.h"
#include "handler_utilities.h"


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
        We need to update our database entry to set the game's pending to false
        Then we need to set up timers for the game and get those rolling
        We'll need some kind of data structure to hold the information that pertains
        to the game, such as the move history and how the timers are doing
        */
       if (game != NULL) {
           game->pending = false;
       }
       else {

       }

    }
    else {
        if (game != NULL) {

        }
        else {

        }
    }
    // Start an instance of a game
    // Send out the game_accept notifs to the players with the game info
}
