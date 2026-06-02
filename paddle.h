#ifndef PADDLE_H
#define PADDLE_H

void paddle_init(int col);
void paddle_up(void);
void paddle_down(void);
int paddle_contact(int y, int x);

#endif
