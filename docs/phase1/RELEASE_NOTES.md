# Release Notes Draft — v0.2-phase1-analysis

## Overview

This release is the design checkpoint for `netPong`. I did not change the gameplay code in this milestone. I used it to map the original Pong logic to the networking RFC and to decide what has to change before multiplayer can work.

## What This Milestone Contains

- a written Phase 1 report
- a breakdown of which parts of the baseline code are reusable
- a concrete plan for court configuration, socket ball transfer, miss handling, and timer control

## Implementation Notes

The important result from this phase is that the project is not a full rewrite. The baseline already separates paddle movement, serve logic, and timed ball updates well enough that I can refactor around those pieces instead of starting over.

## Build and Run

The tagged code still builds and runs like the baseline release:

```bash
make
./pong
```

## Verification

- confirmed the source tree still builds cleanly
- verified that the report matches the current code structure
- checked that the documented changes describe real baseline assumptions

## Limitations

There is still no networking code in this release. This is a documentation milestone.

## Next Step

The next release should only happen after the `phase2` branch contains actual socket and game-state transfer work.
