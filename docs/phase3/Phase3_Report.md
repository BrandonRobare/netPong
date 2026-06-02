# NetPong Phase III Report

Course: CS43203 Systems Programming
Student: Brandon Robare
Project: netpong, Phase III
Date: 2026-04-26

## Purpose and Sources

Phase III implements the full SPPBTP networking protocol designed in Phases I and II.
I used the assignment handout (`Project-Instructions.pdf`), the SPPBTP RFC (`Phase1/rfcNP.pdf`),
the Phase I and Phase II design reports, and the baseline `pong.c` / `paddle.c`.

The result is a single combo program: `netpong <port>` runs as the server; `netpong <host> <port>` runs as the client. Two instances play a networked pong game over TCP.

---

## New and Modified Files

### New files

| File | Purpose |
|---|---|
| `court.h` | Defines `struct court` (paddle_col, net_col, out_x_dir, incoming_x_dir, play_top/bot) and `configure_side()`. Replaces the four hard-coded `#define` constants with runtime values so the same engine can render either side. |
| `net.c` / `net.h` | `make_server_socket(port)`, `wait_for_client(listen_fd)`, `connect_to_server(host, port)`. Encapsulates socket/bind/listen/accept and socket/connect following the `socklib.c` pattern from Molay Ch. 12. |
| `protocol.c` / `protocol.h` | SPPBTP message layer. Sender helpers (`send_helo`, `send_name`, `send_serv`, `send_ball`, `send_miss`, `send_done`, `send_quit`, `send_err`) write CRLF-terminated lines via `dprintf`. `recv_msg(FILE*, msg*)` reads one line, upper-cases the 4-char keyword, and fills a tagged `struct sppbtp_msg`. |

### Modified files

**`paddle.c` / `paddle.h`** — `paddle_init()` now takes an `int col` argument instead of using the hard-coded `PADDLE_COL 70` constant. Callers pass `the_court.paddle_col`.

**`pong.c`** — Primary changes:

1. **argv dispatch** — `main(argc, argv)` routes to server or client mode based on argument count.
2. **court struct** — All references to `TOP_ROW`, `BOT_ROW`, `LEFT_EDGE`, `RIGHT_EDGE` in `draw_court`, `serve_ball`, and `bounce_or_lose` now use `the_court.*` fields.
3. **State machine** — `enum game_state { STATE_INTRO, STATE_PLAY, STATE_WAIT, STATE_FINISHED }` replaces the unconditional single-player loop.
4. **`enter_play(msg)` / `enter_wait(reason)`** — State transition functions that also control the ticker via `set_ticker()`. `enter_play(NULL)` is used after a local serve; `enter_play(&msg)` reconstructs a received ball.
5. **`bounce_or_lose()` net-edge branch** — When the ball reaches the net column, the function calls `send_ball()` and `enter_wait()` instead of reversing `x_dir`.
6. **`bounce_or_lose()` miss branch** — Calls `send_miss()` and `enter_wait()` instead of the local `serve_ball()`.
7. **`run_play_loop()`** — The keyboard/timer loop for STATE_PLAY. Uses `select()` with zero timeout to peek at the socket for an opponent QUIT without blocking.
8. **`run_wait_loop()`** — Blocks on `recv_msg(sock_fp, &msg)` and dispatches BALL → `enter_play`, MISS → local serve or send DONE, DONE → send QUIT + FINISHED, QUIT → FINISHED.
9. **Scoreboard** — `my_score`, `opponent_score`, and `balls_left` displayed on status row 1.

---

## How Sockets Are Used

**Server path:**
```
make_server_socket(port)  →  bind + listen
wait_for_client(listen_fd) →  accept (closes listening socket)
sock_fd = connected fd
sock_fp = fdopen(dup(sock_fd), "r")   ← buffered reads
```

**Client path:**
```
connect_to_server(host, port)  →  socket + connect
sock_fd = connected fd
sock_fp = fdopen(dup(sock_fd), "r")
```

All SPPBTP writes use `dprintf(sock_fd, "...\r\n", ...)`.
All SPPBTP reads use `fgets` inside `recv_msg(sock_fp, &msg)`.
The `dup()` ensures `fclose(sock_fp)` and `close(sock_fd)` do not interfere.

---

## SPPBTP Protocol Mapping (RFC Sections → Code)

| RFC Section | Code |
|---|---|
| §4 HELO / NAME / SERV | `main()` INTRO block in server and client branches |
| §5 BALL | `bounce_or_lose()` net-edge → `send_ball()` + `enter_wait()`; `run_wait_loop()` MSG_BALL → `enter_play(&msg)` |
| §5 MISS | `bounce_or_lose()` miss → `send_miss()` + `enter_wait()`; `run_wait_loop()` MSG_MISS → local serve or `send_done()` |
| §6 DONE / QUIT | `run_wait_loop()` MSG_DONE → `send_quit()`; MSG_QUIT → FINISHED |
| §3 ?ERR | `run_wait_loop()` default → `send_err()` + FINISHED |

---

## Build and Run

```
make
```

**Server (terminal A):**
```
./pong 2001
```
Prints `waiting for connection on port 2001...` before starting curses.

**Client (terminal B):**
```
./pong localhost 2001
```

Both windows start immediately after the TCP connection is established.

---

## Verification

### Build
`make clean && make` — zero warnings with `-Wall -Wextra -pedantic` (see `clean_compile.txt`).

### Two-terminal local game
1. Start server: `./pong 2001`
2. Start client: `./pong localhost 2001`
3. Client serves first (RFC §4 SERV). Ball passes through net into server's court.
4. Server returns. Ball passes back.
5. Miss: scoreboard updates; opponent serves next; `balls_left` decrements on both sides.
6. After 3 misses: DONE/QUIT exchange closes both windows cleanly.
7. `Q` during PLAY: opponent receives quit and both windows exit.

### Protocol conformance via telnet
```
telnet localhost 2001
```
Expected exchange:
```
<- HELO 1.0 20 16 <hostname>
-> NAME 1.0 testbot
<- SERV 3
-> BALL 8 3 4 1 O
...
-> QUIT thanks
```
Malformed input (e.g. `XYZZY`) → server responds `?ERR unknown: XYZZ` and exits.

---

## Limitations

- **`select()` not implemented in WAIT state.** The assignment (p.5 [H]) notes this is acceptable: "If the ball is not in a player's court, the Q key may not register immediately." Keyboard input during WAIT is blocked. This is the described extra-credit feature.
- **Server accepts one game per invocation.** The assignment item [G] ("server goes back to waiting mode") is not implemented. After `wait_for_client()` closes the listening socket, a second game requires restarting the server.

---

## What I Changed

Added `court.h`, `net.c`/`net.h`, and `protocol.c`/`protocol.h` as new modules. Refactored `paddle_init()` to accept a column argument. Rewrote `pong.c`'s `main()`, the `bounce_or_lose()` horizontal branches, and the game loop entirely around the PLAY/WAIT state machine.

## What I Learned

The most surprising design detail was that the server initiates the protocol (sends HELO, SERV) but the *client* serves the first ball — a role reversal from the standard client-initiates model. The second insight was how simple ticker control is: `setitimer` with zero stops it, with `TICK_MSECS` starts it again. There is no new mechanism required; the existing `set_ticker()` already supported pause.
