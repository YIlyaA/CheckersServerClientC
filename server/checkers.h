#ifndef CHECKERS_H
#define CHECKERS_H

#define BOARD_SIZE 32

typedef enum {
    CELL_EMPTY = '.',
    CELL_WHITE = 'w',
    CELL_WHITE_KING = 'W',
    CELL_BLACK = 'b',
    CELL_BLACK_KING = 'B'
} Cell;

typedef enum {
    COLOR_WHITE,
    COLOR_BLACK
} PlayerColor;

typedef enum {
    GAME_RUNNING,
    GAME_WHITE_WIN,
    GAME_BLACK_WIN,
    GAME_DRAW
} GameResult;

typedef struct {
    char board[BOARD_SIZE];
    PlayerColor turn;
    GameResult result;
} Game;

void game_init(Game *g);
void game_print(const Game *g);
int game_is_move_legal(const Game *g, int from, int to);
int game_apply_move(Game *g, int from, int to);
int game_is_finished(Game *g);

#endif