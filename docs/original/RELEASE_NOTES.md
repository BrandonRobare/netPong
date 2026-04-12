# Release Notes Draft — v0.1-original-pong

## Overview

This release captures the original Pong assignment before I started the networking project. I tagged it so I would have a stable reference point for later refactors and so the repo history would show where `netPong` actually started.

## What This Milestone Contains

- a working single-player terminal Pong game
- a separated paddle module in `paddle.c`
- timer-driven ball movement using `SIGALRM`
- build support through the repo `Makefile`

## Implementation Notes

The code is still shaped around a local game. The paddle is fixed on one side, the far wall bounces the ball back, and a missed ball immediately triggers a local re-serve. Those assumptions are exactly what later milestones need to break apart.

## Build and Run

```bash
make
./pong
```

## Verification

- built locally with `gcc`, `-Wall`, `-Wextra`, and `-pedantic`
- verified that the game launches and responds to `k`, `j`, and `Q`

## Limitations

There is no network play in this release. It is the baseline game only.

## Next Step

The next release documents how I planned the networking changes before touching the game logic.
