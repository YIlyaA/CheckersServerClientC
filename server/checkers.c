#include "checkers.h"
#include <stdio.h>

void game_init(Game *g)
{
    for (int i = 0; i < BOARD_SIZE; ++i) {
        g->board[i] = CELL_EMPTY;
    }

    for (int i = 0; i < 12; ++i) {
        g->board[i] = CELL_BLACK;
    }
    for (int i = 20; i < 32; ++i) {
        g->board[i] = CELL_WHITE;
    }

    g->turn = COLOR_WHITE;
    g->result = GAME_RUNNING;
}

void game_print(const Game *g)
{
    printf("Board (32 cells):\n");
    for (int i = 0; i < BOARD_SIZE; ++i) {
        printf("%c ", g->board[i]);
        if ((i + 1) % 8 == 0)
            printf("\n");
    }
    printf("Turn: %s\n", g->turn == COLOR_WHITE ? "WHITE" : "BLACK");
}

int game_is_move_legal(const Game *g, int from, int to)
{
    if (from < 0 || from >= BOARD_SIZE || to < 0 || to >= BOARD_SIZE)
        return 0;
    if (g->board[from] == CELL_EMPTY)
        return 0;
    if (g->board[to] != CELL_EMPTY)
        return 0;

    // TODO: check for legal moves, captures, kings, etc.
    return 1;
}

int game_apply_move(Game *g, int from, int to)
{
    if (!game_is_move_legal(g, from, to))
        return 0;

    g->board[to] = g->board[from];
    g->board[from] = CELL_EMPTY;

    // TODO: check for promotion to king, captures, etc.

    // Switch turn
    g->turn = (g->turn == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
    return 1;
}

int game_is_finished(Game *g)
{
    // TODO: implement actual game finished check
    return (g->result != GAME_RUNNING);
}
