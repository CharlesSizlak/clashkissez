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

/*
All righty now
We can use a bitfield that corresponds to squares on the board stored in BOARD_STATE
We can store an array of pieces that contain arrays of their legal moves
We can use a hashtable to store board states that uses the board state as keys mapped to ints
and we just up the ints by 1 every time a board state is reached and we check for
repitition that way
We need to store a flag that tracks checks
If a check is reached we update all pieces legal moves in the array 
only allowing moves that handle the check
When a check is resolved we update all pieces of that colors legal moves again

We need to be able to check for checkmate and stalemate after every move.
Our bitfield can store the following information to help us do these checks
1 Piece Present
2 Piece is White/Black
3 Square is under attack by a white pawn
4 Square is under attack by a white knight
5 Square is under attack by a white piece that moves like a bishop like this \ bend dexter
6 Square is under attack by a white piece that moves like a bishop like this / bend sinister
7 Square is under attack by a white piece that moves like a rook along a row
8 Square is under attack by a white piece that moves like a rook along a columm
9 Square is under attack by a white king
10 Square is under attack by a black pawn
11 Square is under attack by a black knight
12 Square is under attack by a black piece that moves like a bishop like this \ bend dexter
13 Square is under attack by a black piece that moves like a bishop like this / bend sinister
14 Square is under attack by a black piece that moves like a rook along a row
15 Square is under attack by a black piece that moves like a rook along a columm
16 Square is under attack by a black  king
*/

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

/**
 * @brief Frees resources allocated for a game
 */
void game_free(game_t *game);

#endif
