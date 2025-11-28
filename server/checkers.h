#ifndef CHECKERS_H
#define CHECKERS_H

#define BOARD_SIZE 8

typedef enum
{
    CELL_EMPTY = '.',
    CELL_WHITE = 'w',
    CELL_WHITE_KING = 'W',
    CELL_BLACK = 'b',
    CELL_BLACK_KING = 'B'
} Cell;

typedef enum
{
    COLOR_WHITE,
    COLOR_BLACK
} PlayerColor;

typedef enum
{
    GAME_RUNNING,
    GAME_WHITE_WIN,
    GAME_BLACK_WIN,
    GAME_DRAW
} GameResult;

typedef struct
{
    char board[BOARD_SIZE][BOARD_SIZE];
    PlayerColor turn; // COLOR_WHITE or COLOR_BLACK
    GameResult result; // GAME_RUNNING, GAME_WHITE_WIN, GAME_BLACK_WIN, GAME_DRAW

    int must_continue_capture; // whether the current player must continue a capture sequence
    int cap_row; // row of the piece that must continue capturing
    int cap_col; // column of the piece that must continue capturing
} Game;

void game_init(Game *g);
void game_print(const Game *g);
void game_print_for_player(const Game *g, PlayerColor pov);
int game_is_move_legal(const Game *g, int from_row, int from_col, int to_row, int to_col);
int game_apply_move(Game *g, int from_row, int from_col, int to_row, int to_col);
int game_is_finished(Game *g);

#endif
