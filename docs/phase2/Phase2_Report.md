# NetPong Phase II Report

Course: CS43203 Systems Programming
Student: Brandon Robare
Project: netpong, Phase II
Date: 2026-04-12

## Purpose and Sources

This report answers Phase II questions 5-8 from `Project-Instructions.pdf` as a design document for the later C implementation. I used the assignment handout, the SPPBTP RFC in `Phase1/rfcNP.pdf`, the project notes in the `systemsProgramming` Obsidian vault, the Phase I report, and the current baseline source in `pong.c` and `paddle.c`.

The baseline program is still a one-process curses game. The important design issue for Phase II is deciding which side owns the paddle, the timer, the ball, and the next serve at each moment. The RFC solves that by splitting behavior into `PLAY` and `WAIT` states inside the `PLAYBALL` phase.

## 5. What about the paddle? When the ball is in the other process, do you want to allow your player to move the paddle?

For the basic Phase II design, no. Paddle movement should only be active while the local process is in the `PLAY` state. When the local process is in the `WAIT` state, the other machine owns the PPB, so the local program should stop acting on `j` and `k` and simply wait for network messages. This matches the assignment text, which treats always-available paddle motion and immediate quit handling during `WAIT` as extra credit through `select()` or `poll()`.

Algorithm 5 - handle_input_by_state

```text
handle_input_by_state(state, input_source):
    if state == PLAY:
        if input_source == keyboard_j:
            paddle_down()
        else if input_source == keyboard_k:
            paddle_up()
        else if input_source == keyboard_Q:
            send QUIT
            state = FINISHED
        else if input_source == timer:
            bounce_or_lose()
        else if input_source == socket:
            protocol error unless QUIT

    else if state == WAIT:
        if input_source == socket:
            process BALL, MISS, DONE, or QUIT
        else:
            ignore local keyboard and timer events
```

Chart 5 - input policy by local state

```text
+-------+------------------------+----------------------+---------------------------+
| State | Keyboard j/k           | Timer tick           | Socket input              |
+-------+------------------------+----------------------+---------------------------+
| PLAY  | move paddle            | move ball / bounce   | only exceptional traffic  |
| WAIT  | ignored in base design | disabled             | BALL, MISS, DONE, QUIT    |
+-------+------------------------+----------------------+---------------------------+
```

Tie-back to the current Pong baseline:
The current `main()` loop in `pong.c` lines 65-79 always accepts `j`, `k`, and `Q`, because the game assumes the local process always owns the court. Phase II changes that assumption by making keyboard actions conditional on the local `PLAY` or `WAIT` state. The existing `paddle_up()` and `paddle_down()` functions in `paddle.c` lines 39-60 can stay as they are; the change is deciding when the caller is allowed to invoke them.

## 6. What does your program do when the ball is in your court? What does the program do when the ball is in the other court? How does your program make this transition?

When the ball is in the local court, the program is in the RFC `PLAY` state. In that state, the local machine owns the PPB, runs the ticker, updates motion, bounces off walls and paddle, and eventually either sends `BALL` when the PPB crosses the net or sends `MISS` when the local player misses. When the ball is in the remote court, the local program is in the `WAIT` state. In that state, the local machine does not move the PPB, does not run the physics ticker, and waits for a socket command from the peer. The transition is therefore a transfer of ownership, not just a screen redraw.

Algorithm 6 - enter_play and enter_wait

```text
enter_play(received_ball):
    have_ball = 1
    state = PLAY
    install received ball data into local ppball
    draw ball on local court
    set_ticker(TICK_MSECS)

enter_wait(reason):
    have_ball = 0
    state = WAIT
    erase local ball if needed
    set_ticker(0)
    show waiting message based on reason

on_local_net_crossing():
    send BALL ...
    enter_wait("waiting for return")

on_local_miss():
    send MISS ...
    enter_wait("waiting for serve or DONE")

on_receive_ball():
    enter_play(received BALL data)

on_receive_quit():
    state = FINISHED
```

Chart 6 - local runtime state transitions

```text
                 +-------------------+
                 | INTRODUCTION      |
                 | ticker off        |
                 +---------+---------+
                           |
                      SERV / first BALL
                           |
             +-------------+-------------+
             |                           |
             v                           v
      +-------------+   send BALL   +-------------+
      | PLAY        |-------------->| WAIT        |
      | ticker on   |               | ticker off  |
      +------+------+<--------------+------+------+
             |        receive BALL          |
             |                              |
        send MISS                           | receive QUIT
             |                              |
             v                              v
      +-------------+                 +-------------+
      | WAIT        |                 | FINISHED    |
      +------+------+                 +-------------+
             |
     receive MISS with
     balls_left == 0
             |
             v
      DONE / QUIT closeout
```

Tie-back to the current Pong baseline:
The baseline starts the ticker once in `set_up()` at `pong.c` lines 101-129 and never changes ownership after that. It also always serves locally from `serve_ball()` at lines 177-194. Phase II inserts explicit state transitions around those existing mechanics so the ball, timer, and serve are owned by only one process at a time.

## 7. How does the ball travel from court to court? What does the other process need to know about the ball? What do you need to know from the other process when it sends the ball back?

The ball travels from court to court through the RFC `BALL net_position xttm yttm ydir [PPBchar]` command. The sending process must preserve the ball state that determines how the PPB should continue moving on the other side: its position relative to the top of the net, its horizontal and vertical timing values, its vertical direction, and optionally the display character. The receiving process reconstructs a local `ppball` from those fields, places the ball at its own net edge, restores timing and direction, and changes from `WAIT` to `PLAY`. The major code change is that the far-wall bounce branch is replaced by socket serialization and handoff.

Algorithm 7 - send_ball_to_peer and receive_ball_from_peer

```text
send_ball_to_peer():
    net_position = ball.y_pos - court.play_top
    send "BALL net_position ball.x_ttm ball.y_ttm ball.y_dir ball.symbol"
    erase local ball
    have_ball = 0
    state = WAIT

receive_ball_from_peer(net_position, xttm, yttm, ydir, symbol):
    ball.y_pos = court.play_top + net_position
    ball.x_pos = court.net_entry_col
    ball.x_ttm = xttm
    ball.y_ttm = yttm
    ball.x_ttg = xttm
    ball.y_ttg = yttm
    ball.y_dir = ydir
    ball.x_dir = court.incoming_x_dir
    ball.symbol = symbol or BALL_CHAR
    enter_play(ball)
```

Chart 7 - BALL fields and their meaning

```text
+----------------+----------------------------------+--------------------------------------+
| BALL field     | Sender derives it from          | Receiver uses it for                 |
+----------------+----------------------------------+--------------------------------------+
| net_position   | ball row relative to net top    | local row where PPB enters court     |
| xttm           | current horizontal timing       | local horizontal move rate           |
| yttm           | current vertical timing         | local vertical move rate             |
| ydir           | current vertical direction      | local up/down direction              |
| PPBchar        | optional ball display symbol    | optional local display symbol        |
+----------------+----------------------------------+--------------------------------------+
```

Tie-back to the current Pong baseline:
Right now `bounce_or_lose()` in `pong.c` lines 229-257 treats one horizontal edge as a wall and the other as a paddle check. In netpong, the far-wall case at lines 230-232 stops being a bounce. That branch becomes the place where the program formats and sends a `BALL` command, erases the local PPB, and yields control to the peer.

## 8. How do you keep score?

Each process should track `my_score`, `opponent_score`, and a shared `balls_left` counter. A miss awards a point to the remote side and transfers serve ownership to that side. Therefore, when the local player misses, the local process decrements `balls_left`, increments `opponent_score`, sends `MISS`, and enters `WAIT`. When the waiting side receives `MISS`, it decrements its own copy of `balls_left`, increments `my_score`, and either serves the next PPB or, if no PPB remains, sends `DONE`. The sender of the last `MISS` then acknowledges the `DONE` with `QUIT`, which closes the game cleanly and symmetrically.

Algorithm 8 - handle_local_miss and handle_received_miss

```text
handle_local_miss():
    erase ball
    balls_left = balls_left - 1
    opponent_score = opponent_score + 1
    send "MISS optional_message"

    if balls_left > 0:
        enter_wait("opponent serves next")
    else:
        enter_wait("waiting for DONE")

handle_received_miss():
    balls_left = balls_left - 1
    my_score = my_score + 1
    update scoreboard

    if balls_left > 0:
        serve_ball_toward_net()
        state = PLAY
    else:
        send "DONE good game"
        state = FINISHED_WAITING_FOR_QUIT
```

Chart 8 - scoring and next-serve control flow

```text
local miss
   |
   v
balls_left--
opponent_score++
send MISS
   |
   +--> balls_left > 0 ------> WAIT for opponent serve
   |
   +--> balls_left == 0 -----> WAIT for DONE

peer receives MISS
   |
   v
balls_left--
my_score++
   |
   +--> balls_left > 0 ------> serve new PPB, enter PLAY
   |
   +--> balls_left == 0 -----> send DONE, wait for QUIT
```

Tie-back to the current Pong baseline:
In the baseline, a miss in `bounce_or_lose()` at `pong.c` lines 246-254 decrements `balls_left`, prints a local message, and immediately calls `serve_ball()` again if another ball remains. Phase II removes that local re-serve assumption. The serve after a miss now belongs to the other process, and the scoreboard must reflect that distributed ownership.

## Conclusion

Phase II turns the single-player Pong baseline into a two-state network design. The local machine is either actively simulating the PPB in `PLAY` or passively waiting for protocol commands in `WAIT`. Paddle input, timer ownership, ball transfer, scoring, and next-serve control all follow from that one design choice. This keeps the Phase III C implementation focused: replace the far-wall bounce with `BALL`, replace local re-serve after a miss with `MISS` or `DONE`, and make timer and input handling state-dependent.
