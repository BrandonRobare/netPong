# netPong — CS4-53203 Systems Programming

Course: CS4-53203 Systems Programming  
Program: `pong.c`, `paddle.c`, `paddle.h`

## Overview

This repository tracks how I am turning the original one-player Pong assignment into `netPong`, a networked version of the game for Systems Programming.

I am keeping one source tree at the repo root and using Git tags to mark the major milestones:

- `v0.1-original-pong` — original one-player Pong baseline
- `v0.2-phase1-analysis` — Phase 1 architecture, RFC mapping, and design documentation
- `phase2` branch — active work for the next implementation milestone

## Baseline Features

- Three-sided visible court with corners at `(4,70)`, `(4,9)`, `(21,9)`, `(21,70)`
- Six-row paddle (`#`) at column `70`, controlled by `k` and `j`
- Paddle bounds enforced (`top >= 5`, `bottom <= 20`)
- Ball (`O`) served from court center with randomized speed
- Elastic bounce off top, bottom, and left walls
- `paddle.c` object-style module with `paddle_init()`, `paddle_up()`, `paddle_down()`, and `paddle_contact(y, x)`
- `bounce_or_lose()` returns `0` for no event, `1` for bounce, and `-1` for lose
- Missed balls decrement the remaining-ball counter and trigger a new serve until three balls are used
- `Q` quits the game

## Project Layout

- `pong.c`, `paddle.c`, `paddle.h` — active source tree
- `docs/original/` — release notes and milestone docs for the original Pong baseline
- `docs/phase1/` — Phase 1 analysis materials and release notes
- `docs/phase2/` — Phase 2 release prep and implementation notes

I kept the class handouts, submission files, and scratch folders out of the tracked repo so the public project only contains source and documentation that I wrote.

## Build

```bash
make
```

## Run

```bash
./pong
```

## Next Step

The next milestone is the first real `phase2` release. That tag should not be created until the branch contains actual networking behavior and the release notes can describe working code instead of planned code.
