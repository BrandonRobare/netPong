#include <curses.h>

#include "paddle.h"

#define PADDLE_COL 70
#define PADDLE_TOP_LIMIT 5
#define PADDLE_BOTTOM_LIMIT 20
#define PADDLE_HEIGHT 6
#define PADDLE_CHAR '#'
#define BLANK ' '

struct pppaddle {
    int pad_top;
    int pad_bot;
    int pad_col;
    char pad_char;
};

static struct pppaddle the_pad;

static void draw_paddle(void)
{
    int row;

    for (row = the_pad.pad_top; row <= the_pad.pad_bot; ++row) {
        mvaddch(row, the_pad.pad_col, the_pad.pad_char);
    }
}

void paddle_init(void)
{
    the_pad.pad_col = PADDLE_COL;
    the_pad.pad_char = PADDLE_CHAR;
    the_pad.pad_top = 10;
    the_pad.pad_bot = the_pad.pad_top + (PADDLE_HEIGHT - 1);
    draw_paddle();
}

void paddle_up(void)
{
    if (the_pad.pad_top <= PADDLE_TOP_LIMIT) {
        return;
    }

    mvaddch(the_pad.pad_bot, the_pad.pad_col, BLANK);
    --the_pad.pad_top;
    --the_pad.pad_bot;
    mvaddch(the_pad.pad_top, the_pad.pad_col, the_pad.pad_char);
}

void paddle_down(void)
{
    if (the_pad.pad_bot >= PADDLE_BOTTOM_LIMIT) {
        return;
    }

    mvaddch(the_pad.pad_top, the_pad.pad_col, BLANK);
    ++the_pad.pad_top;
    ++the_pad.pad_bot;
    mvaddch(the_pad.pad_bot, the_pad.pad_col, the_pad.pad_char);
}

int paddle_contact(int y, int x)
{
    if (x != the_pad.pad_col) {
        return 0;
    }

    return y >= the_pad.pad_top && y <= the_pad.pad_bot;
}
