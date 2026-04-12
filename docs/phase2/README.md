# Phase 2 Release Prep

Planned tag: `v0.3-phase2`

This folder is for the first Phase 2 release, but I am treating it as release prep until the branch contains real networking behavior.

## Release Gate

I should not cut `v0.3-phase2` until all of the following are true:

- the code sends or receives game state over sockets
- the local game can enter a waiting state when the ball leaves the court
- the release notes can describe implemented behavior, not planned behavior
- the build instructions and verification notes match the tagged code

## Current Branch State

The `phase2` branch exists, but it is still the setup point for the implementation milestone. I have not tagged it yet because that would overstate what is in the code.
