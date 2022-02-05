#include "models.h"
#include <stdlib.h>

void game_free(game_t *game) {
    free(game->game_id);
    free(game->invited_player);
    free(game->inviting_player);
    free(game->invited_sid);
    free(game->inviting_sid);
    free(game);
}
