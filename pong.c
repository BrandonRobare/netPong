// CS43203: Systems Programming
// Name: Brandon Robare
// Date: 2/27/2026
// Assignment: netpong

#define _DEFAULT_SOURCE

#include <curses.h>
#include <sys/select.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "court.h"
#include "net.h"
#include "paddle.h"
#include "protocol.h"

#define TOP_ROW    4
#define BOT_ROW    21
#define LEFT_EDGE  9
#define RIGHT_EDGE 70

#define TICK_MSECS    50
#define BALL_CHAR     'O'
#define BLANK         ' '
#define BALLS_PER_GAME 3

/* ticks_per_second = 1000 / TICK_MSECS */
#define TICKS_PER_SEC (1000 / TICK_MSECS)

#define NO_CONTACT  0
#define BOUNCE      1
#define LOSE       (-1)

/* --------------------------------------------------------------- */
/* Game state                                                       */
/* --------------------------------------------------------------- */

enum game_state {
    STATE_INTRO,
    STATE_PLAY,
    STATE_WAIT,
    STATE_FINISHED
};

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

static struct ppball  the_ball;
static struct court   the_court;
static enum game_state state = STATE_INTRO;

static int balls_left    = 0;
static int my_score      = 0;
static int opponent_score = 0;
static int sock_fd       = -1;
static FILE *sock_fp     = NULL; /* buffered read side */
static char  peer_name[41] = "opponent";
static char  my_name[41]   = "player";

/* --------------------------------------------------------------- */
/* Forward declarations                                            */
/* --------------------------------------------------------------- */

static void draw_court(void);
static void set_up(void);
static void wrap_up(void);
static int  set_ticker(int n_msecs);
static void serve_ball(void);
static int  bounce_or_lose(void);
static int  random_ttm(int min, int max);
static int  jitter_ttm(int current, int min, int max);
static void show_status(const char *message);
static void on_alarm(int signum);
static void enter_play(struct sppbtp_msg *m);
static void enter_wait(const char *reason);
static void run_play_loop(void);
static void run_wait_loop(void);

/* --------------------------------------------------------------- */
/* configure_side: fill the_court based on which side we defend    */
/* --------------------------------------------------------------- */

void configure_side(struct court *c, int side)
{
    c->top_row  = TOP_ROW;
    c->bot_row  = BOT_ROW;
    c->play_top = TOP_ROW + 1;
    c->play_bot = BOT_ROW - 1;

    if (side == SIDE_LEFT) {
        c->paddle_col     = LEFT_EDGE;
        c->net_col        = RIGHT_EDGE;
        c->out_x_dir      = 1;   /* serve toward right (net) */
        c->incoming_x_dir = -1;  /* ball arrives moving left */
    } else {
        c->paddle_col     = RIGHT_EDGE;
        c->net_col        = LEFT_EDGE;
        c->out_x_dir      = -1;  /* serve toward left (net) */
        c->incoming_x_dir = 1;   /* ball arrives moving right */
    }
}

/* --------------------------------------------------------------- */
/* State transitions                                               */
/* --------------------------------------------------------------- */

static void enter_play(struct sppbtp_msg *m)
{
    if (m) {
        /* reconstruct ball from BALL message */
        the_ball.y_pos = the_court.play_top + m->net_position;
        the_ball.x_pos = the_court.net_col + the_court.incoming_x_dir;
        the_ball.y_ttm = m->yttm;
        the_ball.x_ttm = m->xttm;
        the_ball.y_ttg = m->yttm;
        the_ball.x_ttg = m->xttm;
        the_ball.y_dir = m->ydir;
        the_ball.x_dir = the_court.incoming_x_dir;
        the_ball.symbol = m->ppb_char ? m->ppb_char : BALL_CHAR;
        mvaddch(the_ball.y_pos, the_ball.x_pos, the_ball.symbol);
    }
    state = STATE_PLAY;
    set_ticker(TICK_MSECS);
    show_status("Your court — use j/k");
    refresh();
}

static void enter_wait(const char *reason)
{
    set_ticker(0);
    state = STATE_WAIT;
    show_status(reason);
    refresh();
}

/* --------------------------------------------------------------- */
/* main                                                            */
/* --------------------------------------------------------------- */

int main(int argc, char *argv[])
{
    int  side;
    struct sppbtp_msg msg;
    char hostname[64];

    gethostname(hostname, sizeof(hostname));
    snprintf(my_name, sizeof(my_name), "%.40s", hostname);

    if (argc == 2) {
        /* server mode */
        int port      = atoi(argv[1]);
        int listen_fd = make_server_socket(port);
        if (listen_fd == -1) return 1;
        fprintf(stderr, "waiting for connection on port %d...\n", port);
        sock_fd = wait_for_client(listen_fd);
        if (sock_fd == -1) return 1;
        side = SIDE_RIGHT;
    } else if (argc == 3) {
        /* client mode */
        sock_fd = connect_to_server(argv[1], atoi(argv[2]));
        if (sock_fd == -1) return 1;
        side = SIDE_LEFT;
    } else {
        fprintf(stderr, "usage: %s <port>               (server)\n", argv[0]);
        fprintf(stderr, "       %s <hostname> <port>    (client)\n", argv[0]);
        return 1;
    }

    /* Wrap sock_fd in a FILE* for line-oriented reads (Molay Ch.12 pattern).
     * We dup the fd so we can still use the original fd for writing. */
    sock_fp = fdopen(dup(sock_fd), "r");
    if (!sock_fp) {
        perror("fdopen");
        close(sock_fd);
        return 1;
    }

    configure_side(&the_court, side);
    set_up();

    /* ---- INTRODUCTION phase ---- */
    if (side == SIDE_RIGHT) {
        /* server: send HELO, expect NAME, send SERV, then enter WAIT */
        send_helo(sock_fd, TICKS_PER_SEC,
                  the_court.play_bot - the_court.play_top + 1,
                  my_name);

        /* wait for NAME */
        while (recv_msg(sock_fp, &msg) && msg.kind == MSG_NONE)
            ; /* skip empty lines */
        if (msg.kind == MSG_NAME)
            snprintf(peer_name, sizeof(peer_name), "%s", msg.name);
        else if (msg.kind == MSG_QUIT) {
            wrap_up();
            return 0;
        }

        balls_left = BALLS_PER_GAME;
        send_serv(sock_fd, balls_left);

        /* client serves first — we wait */
        enter_wait("Waiting for client serve...");

    } else {
        /* client: expect HELO, send NAME, expect SERV, then serve */
        while (recv_msg(sock_fp, &msg) && msg.kind == MSG_NONE)
            ;
        if (msg.kind != MSG_HELO) {
            wrap_up();
            return 1;
        }
        snprintf(peer_name, sizeof(peer_name), "%s", msg.name);

        send_name(sock_fd, my_name);

        while (recv_msg(sock_fp, &msg) && msg.kind == MSG_NONE)
            ;
        if (msg.kind != MSG_SERV) {
            wrap_up();
            return 1;
        }
        balls_left = msg.n_balls;

        /* client serves first */
        serve_ball();
        enter_play(NULL); /* NULL = use ball already placed by serve_ball */
    }

    /* ---- PLAYBALL phase ---- */
    while (state != STATE_FINISHED) {
        if (state == STATE_PLAY)
            run_play_loop();
        if (state == STATE_WAIT)
            run_wait_loop();
    }

    wrap_up();
    printf("Final score — Me: %d  %s: %d\n",
           my_score, peer_name, opponent_score);
    return 0;
}

/* --------------------------------------------------------------- */
/* Play loop: runs while we own the ball                           */
/* --------------------------------------------------------------- */

static void run_play_loop(void)
{
    int c;
    struct sppbtp_msg msg;

    while (state == STATE_PLAY) {
        c = getch(); /* timeout(100) set in set_up — returns ERR after 100ms */

        if (c == 'Q') {
            send_quit(sock_fd, "player quit");
            state = STATE_FINISHED;
            return;
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

        /* Non-blocking peek: use select with zero timeout to check if
         * the socket has data before calling recv_msg (which blocks). */
        {
            fd_set rfds;
            struct timeval tv = {0, 0};
            FD_ZERO(&rfds);
            FD_SET(sock_fd, &rfds);
            if (select(sock_fd + 1, &rfds, NULL, NULL, &tv) > 0) {
                if (recv_msg(sock_fp, &msg) && msg.kind == MSG_QUIT) {
                    show_status("Opponent quit.");
                    state = STATE_FINISHED;
                    return;
                }
            }
        }
    }
}

/* --------------------------------------------------------------- */
/* Wait loop: blocks on socket until BALL / MISS / DONE / QUIT     */
/* --------------------------------------------------------------- */

static void run_wait_loop(void)
{
    struct sppbtp_msg msg;

    while (state == STATE_WAIT) {
        if (!recv_msg(sock_fp, &msg)) {
            /* connection dropped */
            state = STATE_FINISHED;
            return;
        }

        switch (msg.kind) {
        case MSG_BALL:
            enter_play(&msg);
            return;

        case MSG_MISS:
            --balls_left;
            ++my_score;
            show_status("Opponent missed — your serve");
            refresh();
            if (balls_left > 0) {
                serve_ball();
                enter_play(NULL);
            } else {
                send_done(sock_fd, "good game");
                state = STATE_FINISHED;
            }
            return;

        case MSG_DONE:
            send_quit(sock_fd, "thanks for playing");
            state = STATE_FINISHED;
            return;

        case MSG_QUIT:
            if (msg.message[0])
                show_status(msg.message);
            state = STATE_FINISHED;
            return;

        case MSG_NONE:
            /* empty line — keep waiting */
            break;

        default:
            /* unexpected command in PLAYBALL state */
            send_err(sock_fd, "unexpected command in PLAYBALL");
            state = STATE_FINISHED;
            return;
        }
    }
}

/* --------------------------------------------------------------- */
/* Court drawing                                                    */
/* --------------------------------------------------------------- */

static void draw_court(void)
{
    int row, col;
    int step = the_court.incoming_x_dir; /* direction from net toward paddle */

    /* top and bottom walls spanning the full court */
    for (col = the_court.net_col;
         col != the_court.paddle_col + step;
         col += step) {
        mvaddch(the_court.top_row, col, '-');
        mvaddch(the_court.bot_row, col, '-');
    }

    /* back wall on the net side */
    for (row = the_court.top_row; row <= the_court.bot_row; ++row)
        mvaddch(row, the_court.net_col, '|');
}

/* --------------------------------------------------------------- */
/* Curses init / teardown                                          */
/* --------------------------------------------------------------- */

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
    paddle_init(the_court.paddle_col);
    show_status("Connecting...");

    srand((unsigned int)getpid());

    sa.sa_handler = on_alarm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    /* ticker starts OFF; enter_play() enables it */
    if (set_ticker(0) == -1) {
        endwin();
        perror("set_ticker");
        exit(1);
    }

    refresh();
}

static void wrap_up(void)
{
    set_ticker(0);
    if (sock_fp)  fclose(sock_fp);
    if (sock_fd != -1) close(sock_fd);
    endwin();
}

/* --------------------------------------------------------------- */
/* Interval timer                                                  */
/* --------------------------------------------------------------- */

static int set_ticker(int n_msecs)
{
    struct itimerval new_timeset;
    long n_sec   = n_msecs / 1000;
    long n_usecs = (n_msecs % 1000) * 1000L;

    new_timeset.it_interval.tv_sec  = n_sec;
    new_timeset.it_interval.tv_usec = n_usecs;
    new_timeset.it_value.tv_sec     = n_sec;
    new_timeset.it_value.tv_usec    = n_usecs;

    return setitimer(ITIMER_REAL, &new_timeset, NULL);
}

/* --------------------------------------------------------------- */
/* Ball helpers                                                    */
/* --------------------------------------------------------------- */

static int random_ttm(int min, int max)
{
    if (max <= min) return min;
    return min + (rand() % (max - min + 1));
}

static int jitter_ttm(int current, int min, int max)
{
    int delta = (rand() % 3) - 1;

    current += delta;
    if (current < min) current = min;
    if (current > max) current = max;
    return current;
}

static void serve_ball(void)
{
    the_ball.y_pos  = (the_court.play_top + the_court.play_bot) / 2;
    the_ball.x_pos  = (the_court.net_col  + the_court.paddle_col) / 2;
    the_ball.y_ttm  = random_ttm(2, 6);
    the_ball.x_ttm  = random_ttm(2, 5);
    the_ball.y_ttg  = the_ball.y_ttm;
    the_ball.x_ttg  = the_ball.x_ttm;
    the_ball.y_dir  = (rand() % 2 == 0) ? 1 : -1;
    the_ball.x_dir  = the_court.out_x_dir;
    the_ball.symbol = BALL_CHAR;

    mvaddch(the_ball.y_pos, the_ball.x_pos, the_ball.symbol);
    show_status("Serving...");
}

/* --------------------------------------------------------------- */
/* Physics tick — called by SIGALRM handler                        */
/* --------------------------------------------------------------- */

static int bounce_or_lose(void)
{
    int moved_y = 0;
    int moved_x = 0;
    int result  = NO_CONTACT;
    int net_pos;

    if (--the_ball.y_ttg == 0) {
        the_ball.y_ttg = the_ball.y_ttm;
        moved_y = 1;
    }

    if (--the_ball.x_ttg == 0) {
        the_ball.x_ttg = the_ball.x_ttm;
        moved_x = 1;
    }

    if (!moved_y && !moved_x)
        return NO_CONTACT;

    mvaddch(the_ball.y_pos, the_ball.x_pos, BLANK);

    /* Vertical bounces */
    if (moved_y) {
        if (the_ball.y_dir == -1 && the_ball.y_pos <= the_court.play_top) {
            the_ball.y_dir = 1;
            result = BOUNCE;
        } else if (the_ball.y_dir == 1 && the_ball.y_pos >= the_court.play_bot) {
            the_ball.y_dir = -1;
            result = BOUNCE;
        }
        the_ball.y_pos += the_ball.y_dir;
    }

    /* Horizontal: net edge or paddle edge */
    if (moved_x) {
        /* Net crossing: ball moving toward net and one cell away from it */
        if (the_ball.x_dir == the_court.out_x_dir &&
            the_ball.x_pos == the_court.net_col + the_court.incoming_x_dir) {
            /* Step 5: send BALL, enter WAIT */
            net_pos = the_ball.y_pos - the_court.play_top;
            send_ball(sock_fd, net_pos,
                      the_ball.x_ttm, the_ball.y_ttm,
                      the_ball.y_dir, the_ball.symbol);
            mvaddch(the_ball.y_pos, the_ball.x_pos, BLANK);
            enter_wait("Waiting for return...");
            return LOSE; /* stop processing this tick */
        }
        /* Paddle edge: ball moving toward paddle, one cell away */
        else if (the_ball.x_dir == the_court.incoming_x_dir &&
                 the_ball.x_pos == the_court.paddle_col + the_court.out_x_dir) {
            if (paddle_contact(the_ball.y_pos, the_court.paddle_col)) {
                the_ball.x_dir = the_court.out_x_dir;
                the_ball.x_ttm = jitter_ttm(the_ball.x_ttm, 1, 8);
                the_ball.y_ttm = jitter_ttm(the_ball.y_ttm, 1, 8);
                if (the_ball.x_ttg > the_ball.x_ttm) the_ball.x_ttg = the_ball.x_ttm;
                if (the_ball.y_ttg > the_ball.y_ttm) the_ball.y_ttg = the_ball.y_ttm;
                result = BOUNCE;
            } else {
                /* Step 6: local miss */
                --balls_left;
                ++opponent_score;
                mvaddch(the_ball.y_pos, the_ball.x_pos, BLANK);
                send_miss(sock_fd, "missed it");
                if (balls_left > 0)
                    enter_wait("Missed — opponent serves");
                else
                    enter_wait("Missed — waiting for DONE");
                return LOSE;
            }
        }
        the_ball.x_pos += the_ball.x_dir;
    }

    mvaddch(the_ball.y_pos, the_ball.x_pos, the_ball.symbol);
    return result;
}

/* --------------------------------------------------------------- */
/* Status line                                                     */
/* --------------------------------------------------------------- */

static void show_status(const char *message)
{
    mvprintw(1, LEFT_EDGE,
             "Me: %d  %s: %d  Balls: %d | %s",
             my_score, peer_name, opponent_score, balls_left, message);
    clrtoeol();
}

/* --------------------------------------------------------------- */
/* SIGALRM handler                                                  */
/* --------------------------------------------------------------- */

static void on_alarm(int signum)
{
    (void)signum;
    if (state != STATE_PLAY)
        return;
    (void)bounce_or_lose();
    refresh();
}
