# NetPong Phase I Report

Course: CS43203 Systems Programming
Student: Brandon Robare
Project: netpong, Phase I
Date: 2026-03-22

## Baseline Review

The current program is a single-player curses pong game implemented in `pong.c` and `paddle.c`. The court geometry is fixed by the constants `TOP_ROW`, `BOT_ROW`, `LEFT_EDGE`, and `RIGHT_EDGE` in `pong.c` lines 17-20. The paddle is hard-coded on the right side by `PADDLE_COL` in `paddle.c` line 5. Ball motion is driven by the interval timer set in `set_up()` and handled by `on_alarm()`, which calls `bounce_or_lose()` every tick.

The important Phase I observation is that the current code already separates vertical motion, horizontal motion, paddle motion, and serve logic. That means the project is not a rewrite. It is mainly a refactor of the hard-coded court geometry plus a change in what happens at the far edge, on a miss, and while waiting for the remote player.

## 1. How hard is it to change the code so the game can appear with the paddle on the left or on the right? Can you replace the constants for paddle and wall positions with variables?

This change is moderate, not difficult. The game logic is mostly reusable, but several places assume a right-side paddle and a left-side wall:

- `pong.c` lines 17-20 fix the court edges at compile time.
- `paddle.c` lines 5-8 fix the paddle column and movement limits.
- `pong.c` line 234 checks paddle contact specifically at `RIGHT_EDGE`.
- `draw_court()` in `pong.c` draws only the left vertical wall.

The clean solution is to replace those fixed values with a runtime court configuration. Then the same game engine can draw either player view by changing one `side` value during initialization.

Algorithm 1 - initialize the court from the player side

```text
configure_side(side):
    court.top_row    = TOP_ROW
    court.bot_row    = BOT_ROW
    court.play_top   = TOP_ROW + 1
    court.play_bot   = BOT_ROW - 1

    if side == LEFT_PLAYER:
        court.paddle_col = LEFT_EDGE
        court.net_col    = RIGHT_EDGE
        court.out_dir    = +1
    else:
        court.paddle_col = RIGHT_EDGE
        court.net_col    = LEFT_EDGE
        court.out_dir    = -1

    paddle.pad_col = court.paddle_col
    paddle.pad_top = 10
    paddle.pad_bot = paddle.pad_top + PADDLE_HEIGHT - 1
```

Chart 1 - geometry after refactoring

```text
+-------------------+----------------+-------------+------------------+
| View              | Defended Edge  | Net Edge    | Serve Direction  |
+-------------------+----------------+-------------+------------------+
| Current pong      | RIGHT_EDGE     | none        | +1               |
| Left player court | LEFT_EDGE      | RIGHT_EDGE  | +1 toward net    |
| Right player court| RIGHT_EDGE     | LEFT_EDGE   | -1 toward net    |
+-------------------+----------------+-------------+------------------+
```

Conclusion for Question 1:
Once the edge positions become fields in a `court` structure, the same paddle code and most of the ball code can support both views. The main refactor is replacing hard-coded edge constants with `court.paddle_col`, `court.net_col`, and the playable top and bottom rows.

## 2. Look through your pong code and review how the program works. Look at the part where the ball bounces off the far wall. That part will need to be changed. In the new version, the ball will not bounce but will instead be passed through the socket to the other process.

In the current single-player game, the far-wall bounce happens in `bounce_or_lose()` at `pong.c` lines 229-233:

- If `x_dir == -1` and the ball reaches `LEFT_EDGE + 1`, the code reverses `x_dir`.
- That is correct for local pong, but it is wrong for netpong.

In netpong, the far edge is no longer a solid wall. It becomes the net. When the ball reaches that edge, the local process must:

1. Stop drawing the ball locally.
2. Send the ball state to the other process over TCP.
3. Switch from PLAY to WAIT.

The RFC says the `BALL` command must carry:

- the position of the ball relative to the top of the net
- `xttm`
- `yttm`
- `ydir`
- optionally the ball character

So the current far-wall branch should be replaced by a transfer branch.

Algorithm 2 - replace far-wall bounce with BALL transfer

```text
handle_horizontal_event():
    next_x = ball.x_pos + ball.x_dir

    if next_x == court.paddle_col:
        if paddle_contact(ball.y_pos, court.paddle_col):
            ball.x_dir = -ball.x_dir
            jitter ball speed
        else:
            handle_local_miss()

    else if next_x == court.net_col:
        net_pos = ball.y_pos - court.play_top
        send "BALL net_pos ball.x_ttm ball.y_ttm ball.y_dir ball.symbol"
        erase ball from screen
        have_ball = 0
        state = WAIT

    else:
        ball.x_pos = next_x
```

Chart 2 - horizontal decision flow in netpong

```text
             +--------------------------+
tick with x  | compute next horizontal  |
movement --->| position (next_x)        |
             +------------+-------------+
                          |
          +---------------+---------------+
          |                               |
 next_x == paddle_col?             next_x == net_col?
          |                               |
        yes                               yes
          |                               |
    +-----+-----+                   +-----+------+
    | paddle hit?|                   | send BALL |
    +--+-------+-+                   | erase ball|
       |       |                     | WAIT      |
      yes      no                    +------------+
       |       |
  bounce   send MISS
           WAIT
```

Conclusion for Question 2:
The important design change is that the horizontal edge logic becomes role-based. One edge is the defended side, and the other edge is the transfer side. The transfer side does not reverse `x_dir`; it serializes the ball and hands control to the peer.

## 3. Look at the part where the ball misses the paddle. In the original game, missing the paddle caused the game to serve a new ball. In the new version, the other player serves the new ball.

The current miss logic appears in `pong.c` lines 246-254. Right now the local program does all of the following:

- decrements `balls_left`
- prints a miss message
- immediately calls `serve_ball()` if any balls remain

That behavior must change for netpong. A miss means the opponent scored and the opponent now owns the next serve. Therefore the local process must not call `serve_ball()` after a miss. It must report the miss and wait.

This part also affects scoring. Each process should track:

- `my_score`
- `opponent_score`
- `balls_left`

Both programs must keep the same `balls_left` count. They start from the value sent in `SERV n`, then each miss reduces the shared count by one on both sides.

Algorithm 3 - local miss and remote serve

```text
handle_local_miss():
    erase ball
    balls_left = balls_left - 1
    opponent_score = opponent_score + 1
    send "MISS optional_message"
    have_ball = 0

    if balls_left > 0:
        state = WAIT_FOR_BALL
    else:
        state = WAIT_FOR_DONE

on_receive_miss():
    balls_left = balls_left - 1
    my_score = my_score + 1

    if balls_left > 0:
        serve_ball_toward_net()
        state = PLAY
    else:
        send "DONE good game"
        state = WAIT_FOR_QUIT
```

Chart 3 - miss and serve ownership

```text
Local player misses
        |
        v
 decrement balls_left
 increment opponent_score
 send MISS
        |
        +--> balls_left > 0 ? ---- yes ----> WAIT for opponent serve
        |                               
        +--> no -------------------------> WAIT for DONE

Remote player receives MISS
        |
        v
 decrement balls_left
 increment my_score
        |
        +--> balls_left > 0 ? ---- yes ----> serve new ball
        |
        +--> no -------------------------> send DONE and wait for QUIT
```

Conclusion for Question 3:
This is a control-ownership change, not just a scoring change. The process that misses becomes the waiting side. The other process becomes the serving side, which keeps the game symmetric and matches the RFC.

## 4. Think about the timer. When the ball is on the other court, the ball moves under the control of a different ticker. Do you need to keep your ticker running while the ball is in the other process?

For ball motion, no. The local movement ticker only needs to run while the local process owns the ball. In the current program the timer starts in `set_up()` and stays active until shutdown. `on_alarm()` always calls `bounce_or_lose()`. That is fine for single-player pong but wasteful for netpong because there will be long periods where the ball is not on the local court.

The cleaner design is:

- PLAY state: ticker on, because the ball is moving locally.
- WAIT state: ticker off, because the local process is just waiting for `BALL`, `MISS`, `DONE`, or `QUIT`.

If I later do the extra-credit `select()` or `poll()` version, I would still separate "physics clock" from "input/socket readiness". The physics ticker would stay off in WAIT, but the main loop could still watch the keyboard and socket at the same time.

Algorithm 4 - timer ownership by state

```text
enter_play():
    have_ball = 1
    set_ticker(TICK_MSECS)

enter_wait():
    have_ball = 0
    set_ticker(0)

on_alarm():
    if have_ball == 0:
        return
    bounce_or_lose()
    refresh()
```

Chart 4 - state and ticker chart

```text
            +-----------+
            | INTRO     |
            | ticker=off|
            +-----+-----+
                  |
          connection + SERV/NAME
                  |
        +---------+---------+
        |                   |
        v                   v
  +-----------+       +-----------+
  | PLAY      |       | WAIT      |
  | ticker=on |<----->| ticker=off|
  +-----+-----+ BALL  +-----+-----+
        |                   ^
        | MISS sent         | MISS received and
        | or BALL sent      | balls_left > 0
        v                   |
     WAIT state-------------+

From PLAY or WAIT:
receive QUIT -> FINISHED

Endgame:
WAIT_FOR_DONE + DONE -> send QUIT -> FINISHED
WAIT_FOR_QUIT + QUIT -> FINISHED
```

Conclusion for Question 4:
The timer should belong to the side that currently owns the ball. That keeps the code simple, avoids unnecessary signals, and matches the split-court model where only one process is simulating the ball at a time.

## Final Phase I Summary

The existing single-player program is a solid starting point for netpong. The main work in Phase I is not rewriting the game loop, but refactoring three assumptions out of the current code:

- the paddle is always on the right
- the far edge always bounces the ball
- the same process always serves after a miss

Once those assumptions are replaced with runtime court state, message-based ball transfer, and play/wait state transitions, the program is ready for the full network protocol work in later phases.
