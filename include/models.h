#ifndef _MODELS_H
#define _MODELS_H

#include <stdint.h>
#include "chess.h"

typedef struct game_t {
    bool pending;
    char *game_id;
    char *inviting_player;
    char *invited_player;
    uint32_t time_control_sender;
    uint32_t time_control_receiver;
    uint32_t time_increment_sender;
    uint32_t time_increment_receiver;
    Color_e inviting_player_color;
} game_t;

typedef struct game_timers_t {
    
} game_timers_t;

#endif
