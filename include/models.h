#ifndef _MODELS_H
#define _MODELS_H

#include <stdint.h>
#include "chess.h"

#define BOARD_SIZE 64
#define MAX_MOVES 512

#define ROW(i) ((i) / 8)
#define COLUMN(i) ((i) % 8)

typedef enum piece_e {
    PIECE_NONE = 0x00,
    PIECE_WHITE = 0x10,
    PIECE_BLACK = 0x20,
    PIECE_WHITE_KING = 0x11,
    PIECE_WHITE_QUEEN = 0x12,
    PIECE_WHITE_ROOK = 0x13,
    PIECE_WHITE_BISHOP = 0x14,
    PIECE_WHITE_KNIGHT = 0x15,
    PIECE_WHITE_PAWN = 0x16,
    PIECE_BLACK_KING = 0x21,
    PIECE_BLACK_QUEEN = 0x22,
    PIECE_BLACK_ROOK = 0x23,
    PIECE_BLACK_BISHOP = 0x24,
    PIECE_BLACK_KNIGHT = 0x25,
    PIECE_BLACK_PAWN = 0x26
} piece_e;

typedef struct game_t {
    bool pending;
    char *game_id;
    char *inviting_player;
    char *invited_player;
    char *inviting_sid;
    char *invited_sid;
    uint32_t time_control_sender;
    uint32_t time_control_receiver;
    uint32_t time_increment_sender;
    uint32_t time_increment_receiver;
    size_t timer_id_sender;
    size_t timer_id_receiver;
    Color_e inviting_player_color;
    uint8_t board_state[BOARD_SIZE];
    uint16_t move_history[MAX_MOVES];
    size_t moves_made;
    bool white_short_castle_rights;
    bool white_long_castle_rights;
    bool black_short_castle_rights;
    bool black_long_castle_rights;
} game_t;

typedef struct game_timers_t {
    // TODO figure out what this is lmao (prolly outdated)
} game_timers_t;

void game_free(game_t *game);

#endif