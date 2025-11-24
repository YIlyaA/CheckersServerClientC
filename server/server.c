#include "checkers.h"

int main(void)
{
    Game g;
    game_init(&g);
    game_print(&g);

    game_apply_move(&g, 21, 17);
    game_print(&g);

    return 0;
}