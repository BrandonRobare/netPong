#ifndef COURT_H
#define COURT_H

#define SIDE_RIGHT 0
#define SIDE_LEFT  1

struct court {
    int top_row;
    int bot_row;
    int play_top;
    int play_bot;
    int paddle_col;
    int net_col;
    int out_x_dir;      /* direction ball travels toward net when serving */
    int incoming_x_dir; /* direction ball travels when arriving from net */
};

void configure_side(struct court *c, int side);

#endif
