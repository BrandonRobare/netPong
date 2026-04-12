# Original Pong Milestone

Tag: `v0.1-original-pong`

This milestone is the starting point for the whole project. It is the single-player Pong assignment before any networking work.

## What is in this milestone

- terminal drawing with `ncurses`
- timer-driven ball movement with `SIGALRM`
- paddle movement in `paddle.c`
- wall bounce and loss handling in `pong.c`

## Why I kept it as a separate release

I wanted a clean baseline that I could compare against later phases. The point of this release is not networking. The point is to preserve the working local game that the rest of the project grows from.

## Build and Run

```bash
make
./pong
```

## Limits

This version is single-player only. The ball always stays on the local court. There is no socket code, no peer state, and no remote handoff.
