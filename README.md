# netPong — CS4-53203 Systems Programming

Course: CS4-53203 Systems Programming  
Program: `pong.c`, `paddle.c`, `paddle.h`

## Overview

This repository tracks the evolution of the original one-player terminal Pong assignment into **netPong**, a multiplayer networking project built in phases for Systems Programming.

The root codebase stays as a single evolving implementation, while Git tags mark the major milestones:

- `v0.1-original-pong` — original one-player Pong baseline
- `v0.2-phase1-analysis` — Phase 1 architecture, RFC mapping, and design documentation
- `phase2` branch — active networking implementation work

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
- `docs/phase1/` — Phase 1 analysis materials tracked in Git
- `Phase1/`, `Phase2/`, `Programming Assignment pong/` — local source/reference folders kept outside the tracked milestone history

## Build

```bash
make
```

## Run

```bash
./pong
```

## Next Step

Phase 2 extends this baseline with BSD sockets and multiplayer game-state handoff based on the Phase 1 RFC analysis.
