#include "contexts.h"
#include "database.h"
#include "loop.h"
#include "chess.h"
#include "hash.h"
#include "security.h"
#include "handler_utilities.h"


void resolve_game_invite_handler(loop_t *loop, int fd, request_ctx_t *ctx) {
    // Validate their session token
    bson_oid_t *oid;
    VALIDATE_SESSION_TOKEN(ResolveGameInviteRequest, oid);

    // Check the accepted_status bool to see if it's been accepted or declined
    // Using game_invite_id update the table in the database with the appropriate info
    // Start an instance of a game
    // Send out the game_accept notifs to the players with the game info
}
