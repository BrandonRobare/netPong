// CS43203: Systems Programming
// Name: Brandon Robare
// Date: 2/27/2026
// Assignment: pong

#define _DEFAULT_SOURCE

#include <curses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include "paddle.h"

#define TOP_ROW 4
#define BOT_ROW 21
#define LEFT_EDGE 9
#define RIGHT_EDGE 70

#define TICK_MSECS 50
#define BALL_CHAR 'O'
#define BLANK ' '

#define BALLS_PER_GAME 3

#define NO_CONTACT 0
#define BOUNCE 1
#define LOSE (-1)

struct ppball {
    int y_pos;
    int x_pos;
    int y_ttm;
    int x_ttm;
    int y_ttg;
    int x_ttg;
    int y_dir;
    int x_dir;
    int symbol;
};

static struct ppball the_ball;
static int balls_left = BALLS_PER_GAME;

static void draw_court(void);
static void set_up(void);
static void wrap_up(void);
static int set_ticker(int n_msecs);
static void serve_ball(void);
static int bounce_or_lose(void);
static int random_ttm(int min, int max);
static int jitter_ttm(int current, int min, int max);
static void show_status(const char *message);
static void on_alarm(int signum);

int main(void)
{
    int c;

    set_up();
    serve_ball();

    while (balls_left > 0) {
        c = getch();
        if (c == 'Q') {
            break;
        }
        if (c == 'j') {
            paddle_down();
            (void)bounce_or_lose();
            refresh();
        } else if (c == 'k') {
            paddle_up();
            (void)bounce_or_lose();
            refresh();
        }
    }

    wrap_up();
    printf("Game over.\n");
    return 0;
}

static void draw_court(void)
{
    int row;
    int col;

    for (col = LEFT_EDGE; col <= RIGHT_EDGE; ++col) {
        mvaddch(TOP_ROW, col, '-');
        mvaddch(BOT_ROW, col, '-');
    }

    for (row = TOP_ROW; row <= BOT_ROW; ++row) {
        mvaddch(row, LEFT_EDGE, '|');
    }
}

static void set_up(void)
{
    struct sigaction sa;

    initscr();
    cbreak();
    noecho();
    curs_set(0);
    timeout(100);

    clear();
    draw_court();
    paddle_init();
    show_status("Use j/k to move paddle, Q to quit");

    srand((unsigned int)getpid());

    sa.sa_handler = on_alarm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    if (set_ticker(TICK_MSECS) == -1) {
        endwin();
        perror("set_ticker");
        exit(1);
    }

    refresh();
}

static void wrap_up(void)
{
    (void)set_ticker(0);
    endwin();
}

static int set_ticker(int n_msecs)
{
    struct itimerval new_timeset;
    long n_sec;
    long n_usecs;

    n_sec = n_msecs / 1000;
    n_usecs = (n_msecs % 1000) * 1000L;

    new_timeset.it_interval.tv_sec = n_sec;
    new_timeset.it_interval.tv_usec = n_usecs;
    new_timeset.it_value.tv_sec = n_sec;
    new_timeset.it_value.tv_usec = n_usecs;

    return setitimer(ITIMER_REAL, &new_timeset, NULL);
}

static int random_ttm(int min, int max)
{
    if (max <= min) {
        return min;
    }
    return min + (rand() % (max - min + 1));
}

static int jitter_ttm(int current, int min, int max)
{
    int delta = (rand() % 3) - 1;

    current += delta;
    if (current < min) {
        current = min;
    }
    if (current > max) {
        current = max;
    }
    return current;
}

static void serve_ball(void)
{
    the_ball.y_pos = (TOP_ROW + BOT_ROW) / 2;
    the_ball.x_pos = (LEFT_EDGE + RIGHT_EDGE) / 2;

    the_ball.y_ttm = random_ttm(2, 6);
    the_ball.x_ttm = random_ttm(2, 5);

    the_ball.y_ttg = the_ball.y_ttm;
    the_ball.x_ttg = the_ball.x_ttm;

    the_ball.y_dir = (rand() % 2 == 0) ? 1 : -1;
    the_ball.x_dir = 1;
    the_ball.symbol = BALL_CHAR;

    mvaddch(the_ball.y_pos, the_ball.x_pos, the_ball.symbol);
    show_status("Use j/k to move paddle, Q to quit");
}

static int bounce_or_lose(void)
{
    int moved_y = 0;
    int moved_x = 0;
    int result = NO_CONTACT;

    if (--the_ball.y_ttg == 0) {
        the_ball.y_ttg = the_ball.y_ttm;
        moved_y = 1;
    }

    if (--the_ball.x_ttg == 0) {
        the_ball.x_ttg = the_ball.x_ttm;
        moved_x = 1;
    }

    if (!moved_y && !moved_x) {
        return NO_CONTACT;
    }

    mvaddch(the_ball.y_pos, the_ball.x_pos, BLANK);

    if (moved_y) {
        if (the_ball.y_dir == -1 && the_ball.y_pos <= TOP_ROW + 1) {
            the_ball.y_dir = 1;
            result = BOUNCE;
        } else if (the_ball.y_dir == 1 && the_ball.y_pos >= BOT_ROW - 1) {
            the_ball.y_dir = -1;
            result = BOUNCE;
        }
        the_ball.y_pos += the_ball.y_dir;
    }

    if (moved_x) {
        if (the_ball.x_dir == -1 && the_ball.x_pos <= LEFT_EDGE + 1) {
            the_ball.x_dir = 1;
            result = BOUNCE;
        } else if (the_ball.x_dir == 1 && the_ball.x_pos >= RIGHT_EDGE - 1) {
            if (paddle_contact(the_ball.y_pos, RIGHT_EDGE)) {
                the_ball.x_dir = -1;
                the_ball.x_ttm = jitter_ttm(the_ball.x_ttm, 1, 8);
                the_ball.y_ttm = jitter_ttm(the_ball.y_ttm, 1, 8);

                if (the_ball.x_ttg > the_ball.x_ttm) {
                    the_ball.x_ttg = the_ball.x_ttm;
                }
                if (the_ball.y_ttg > the_ball.y_ttm) {
                    the_ball.y_ttg = the_ball.y_ttm;
                }
                result = BOUNCE;
            } else {
                --balls_left;
                if (balls_left > 0) {
                    show_status("Missed. New ball served.");
                    serve_ball();
                } else {
                    show_status("No balls left.");
                }
                return LOSE;
            }
        }
        the_ball.x_pos += the_ball.x_dir;
    }

    mvaddch(the_ball.y_pos, the_ball.x_pos, the_ball.symbol);
    return result;
}

static void show_status(const char *message)
{
    mvprintw(1, LEFT_EDGE, "Balls left: %d | %s", balls_left, message);
    clrtoeol();
}

static void on_alarm(int signum)
{
    (void)signum;
    (void)bounce_or_lose();
    refresh();
}
