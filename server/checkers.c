#include "checkers.h"
#include <stdio.h>
#include <stdlib.h>

static int is_dark_square(int r, int c)
{
    return (r >= 0 && r < BOARD_SIZE &&
            c >= 0 && c < BOARD_SIZE &&
            ((r + c) % 2 == 1));
}

static PlayerColor piece_color(char p)
{
    if (p == CELL_WHITE || p == CELL_WHITE_KING)
        return COLOR_WHITE;
    if (p == CELL_BLACK || p == CELL_BLACK_KING)
        return COLOR_BLACK;
    return COLOR_WHITE;
}

static int is_king(char p)
{
    return (p == CELL_WHITE_KING || p == CELL_BLACK_KING);
}

void game_init(Game *g)
{
    for (int r = 0; r < BOARD_SIZE; ++r)
    {
        for (int c = 0; c < BOARD_SIZE; ++c)
        {
            g->board[r][c] = CELL_EMPTY;
        }
    }

    for (int r = 0; r < 3; ++r)
    {
        for (int c = 0; c < BOARD_SIZE; ++c)
        {
            if (is_dark_square(r, c))
            {
                g->board[r][c] = CELL_BLACK;
            }
        }
    }

    for (int r = BOARD_SIZE - 3; r < BOARD_SIZE; ++r)
    {
        for (int c = 0; c < BOARD_SIZE; ++c)
        {
            if (is_dark_square(r, c))
            {
                g->board[r][c] = CELL_WHITE;
            }
        }
    }

    g->turn = COLOR_WHITE;
    g->result = GAME_RUNNING;

    g->must_continue_capture = 0;
    g->cap_row = -1;
    g->cap_col = -1;
}


static int can_capture_from(const Game *g, int r, int c)
{
    char piece = g->board[r][c];
    if (piece == CELL_EMPTY)
        return 0;

    PlayerColor pc = piece_color(piece);
    int king = is_king(piece);

    int dirs[4][2];
    int dir_count = 0;

    if (king)
    {
        dirs[0][0] = -2;
        dirs[0][1] = -2;
        dirs[1][0] = -2;
        dirs[1][1] = 2;
        dirs[2][0] = 2;
        dirs[2][1] = -2;
        dirs[3][0] = 2;
        dirs[3][1] = 2;
        dir_count = 4;
    }
    else
    {
        if (pc == COLOR_WHITE)
        {
            dirs[0][0] = -2;
            dirs[0][1] = -2;
            dirs[1][0] = -2;
            dirs[1][1] = 2;
        }
        else
        {
            dirs[0][0] = 2;
            dirs[0][1] = -2;
            dirs[1][0] = 2;
            dirs[1][1] = 2;
        }
        dir_count = 2;
    }

    for (int i = 0; i < dir_count; ++i)
    {
        int dr = dirs[i][0];
        int dc = dirs[i][1];
        int to_r = r + dr;
        int to_c = c + dc;

        if (!is_dark_square(to_r, to_c))
            continue;

        if (g->board[to_r][to_c] != CELL_EMPTY)
            continue;

        int mid_r = r + dr / 2;
        int mid_c = c + dc / 2;

        if (!is_dark_square(mid_r, mid_c))
            continue;

        char mid_piece = g->board[mid_r][mid_c];
        if (mid_piece == CELL_EMPTY)
            continue;

        PlayerColor mid_color = piece_color(mid_piece);
        if (mid_color == pc)
            continue;

        return 1;
    }

    return 0;
}

static int player_has_capture(const Game *g, PlayerColor color)
{
    for (int r = 0; r < BOARD_SIZE; ++r)
    {
        for (int c = 0; c < BOARD_SIZE; ++c)
        {
            char piece = g->board[r][c];
            if (piece == CELL_EMPTY)
                continue;
            if (piece_color(piece) != color)
                continue;
            if (can_capture_from(g, r, c))
                return 1;
        }
    }
    return 0;
}

static int can_simple_move_from(const Game *g, int r, int c)
{
    char piece = g->board[r][c];
    if (piece == CELL_EMPTY)
        return 0;

    PlayerColor pc = piece_color(piece);
    int king = is_king(piece);

    int dirs[4][2];
    int dir_count = 0;

    if (king)
    {
        dirs[0][0] = -1;
        dirs[0][1] = -1;
        dirs[1][0] = -1;
        dirs[1][1] = 1;
        dirs[2][0] = 1;
        dirs[2][1] = -1;
        dirs[3][0] = 1;
        dirs[3][1] = 1;
        dir_count = 4;
    }
    else
    {
        if (pc == COLOR_WHITE)
        {
            dirs[0][0] = -1;
            dirs[0][1] = -1;
            dirs[1][0] = -1;
            dirs[1][1] = 1;
        }
        else
        {
            dirs[0][0] = 1;
            dirs[0][1] = -1;
            dirs[1][0] = 1;
            dirs[1][1] = 1;
        }
        dir_count = 2;
    }

    for (int i = 0; i < dir_count; ++i)
    {
        int nr = r + dirs[i][0];
        int nc = c + dirs[i][1];

        if (!is_dark_square(nr, nc))
            continue;
        if (g->board[nr][nc] != CELL_EMPTY)
            continue;

        return 1;
    }
    return 0;
}

static int player_has_any_move(const Game *g, PlayerColor color)
{
    if (player_has_capture(g, color))
        return 1;

    for (int r = 0; r < BOARD_SIZE; ++r)
    {
        for (int c = 0; c < BOARD_SIZE; ++c)
        {
            char piece = g->board[r][c];
            if (piece == CELL_EMPTY)
                continue;
            if (piece_color(piece) != color)
                continue;
            if (can_simple_move_from(g, r, c))
                return 1;
        }
    }
    return 0;
}

static void update_game_result(Game *g)
{
    int white_count = 0;
    int black_count = 0;

    for (int r = 0; r < BOARD_SIZE; ++r)
    {
        for (int c = 0; c < BOARD_SIZE; ++c)
        {
            char p = g->board[r][c];
            if (p == CELL_WHITE || p == CELL_WHITE_KING)
                white_count++;
            else if (p == CELL_BLACK || p == CELL_BLACK_KING)
                black_count++;
        }
    }

    if (white_count == 0 && black_count == 0)
    {
        g->result = GAME_DRAW;
        return;
    }
    if (white_count == 0)
    {
        g->result = GAME_BLACK_WIN;
        return;
    }
    if (black_count == 0)
    {
        g->result = GAME_WHITE_WIN;
        return;
    }

    int white_moves = player_has_any_move(g, COLOR_WHITE);
    int black_moves = player_has_any_move(g, COLOR_BLACK);

    if (!white_moves && !black_moves)
    {
        g->result = GAME_DRAW;
    }
    else if (!white_moves)
    {
        g->result = GAME_BLACK_WIN;
    }
    else if (!black_moves)
    {
        g->result = GAME_WHITE_WIN;
    }
    else
    {
        g->result = GAME_RUNNING;
    }
}

int game_is_move_legal(const Game *g, int from_row, int from_col,
                       int to_row, int to_col)
{
    if (from_row < 0 || from_row >= BOARD_SIZE ||
        from_col < 0 || from_col >= BOARD_SIZE ||
        to_row < 0 || to_row >= BOARD_SIZE ||
        to_col < 0 || to_col >= BOARD_SIZE)
        return 0;

    if (!is_dark_square(from_row, from_col) ||
        !is_dark_square(to_row, to_col))
        return 0;

    char piece = g->board[from_row][from_col];
    char dest = g->board[to_row][to_col];

    if (piece == CELL_EMPTY)
        return 0;

    if (dest != CELL_EMPTY)
        return 0;

    PlayerColor pc = piece_color(piece);
    if (pc != g->turn)
        return 0;

    int dr = to_row - from_row;
    int dc = to_col - from_col;
    int abs_dr = (dr < 0) ? -dr : dr;
    int abs_dc = (dc < 0) ? -dc : dc;

    if (abs_dr != abs_dc)
        return 0;

    int king = is_king(piece);

    int is_capture_move = (abs_dr == 2);
    int is_simple_move = (abs_dr == 1);

    if (g->must_continue_capture)
    {
        if (from_row != g->cap_row || from_col != g->cap_col)
            return 0;
        if (!is_capture_move)
            return 0;
    }

    if (is_simple_move && player_has_capture(g, g->turn))
    {
        return 0;
    }

    if (is_simple_move)
    {
        if (!king)
        {
            if (pc == COLOR_WHITE && dr != -1)
                return 0;
            if (pc == COLOR_BLACK && dr != 1)
                return 0;
        }
        return 1;
    }

    if (is_capture_move)
    {
        int mid_row = (from_row + to_row) / 2;
        int mid_col = (from_col + to_col) / 2;
        char mid_piece = g->board[mid_row][mid_col];

        if (mid_piece == CELL_EMPTY)
            return 0;

        PlayerColor mid_color = piece_color(mid_piece);
        if (mid_color == pc)
            return 0;

        if (!king)
        {
            if (pc == COLOR_WHITE && dr != -2)
                return 0;
            if (pc == COLOR_BLACK && dr != 2)
                return 0;
        }

        return 1;
    }

    return 0;
}

int game_apply_move(Game *g, int from_row, int from_col,
                    int to_row, int to_col)
{
    if (!game_is_move_legal(g, from_row, from_col, to_row, to_col))
        return 0;

    char piece = g->board[from_row][from_col];
    int dr = to_row - from_row;
    int abs_dr = (dr < 0) ? -dr : dr;

    int was_capture = 0;

    if (abs_dr == 2)
    {
        int mid_row = (from_row + to_row) / 2;
        int mid_col = (from_col + to_col) / 2;
        g->board[mid_row][mid_col] = CELL_EMPTY;
        was_capture = 1;
    }

    g->board[to_row][to_col] = piece;
    g->board[from_row][from_col] = CELL_EMPTY;

    if (piece == CELL_WHITE && to_row == 0)
    {
        g->board[to_row][to_col] = CELL_WHITE_KING;
    }
    else if (piece == CELL_BLACK && to_row == BOARD_SIZE - 1)
    {
        g->board[to_row][to_col] = CELL_BLACK_KING;
    }

    if (was_capture && can_capture_from(g, to_row, to_col))
    {
        g->must_continue_capture = 1;
        g->cap_row = to_row;
        g->cap_col = to_col;
    }
    else
    {
        g->must_continue_capture = 0;
        g->cap_row = -1;
        g->cap_col = -1;

        g->turn = (g->turn == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
    }

    update_game_result(g);

    return 1;
}

int game_is_finished(Game *g)
{
    return (g->result != GAME_RUNNING);
}
