# Pong — Original Baseline

Course: CS4-53203 Systems Programming  
Program: `pong.c`, `paddle.c`, `paddle.h`

## Overview

This project is the original one-player terminal Pong assignment implemented with `curses`, `SIGALRM`, and interval timers.

Implemented features:

- Three-sided visible court with corners at `(4,70)`, `(4,9)`, `(21,9)`, `(21,70)`
- Six-row paddle (`#`) at column `70`, controlled by `k` and `j`
- Paddle bounds enforced (`top >= 5`, `bottom <= 20`)
- Ball (`O`) served from court center with randomized speed
- Elastic bounce off top, bottom, and left walls
- `paddle.c` object-style module with `paddle_init()`, `paddle_up()`, `paddle_down()`, and `paddle_contact(y, x)`
- `bounce_or_lose()` returns `0` for no event, `1` for bounce, and `-1` for lose
- Missed balls decrement the remaining-ball counter and trigger a new serve until three balls are used
- `Q` quits the game

## Build

```bash
make
```

## Run

```bash
./pong
```

## Controls

- `k` move paddle up
- `j` move paddle down
- `Q` quit
