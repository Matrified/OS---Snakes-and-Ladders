Snakes & Ladders (OS Assignment)
Hadi Abdulla
(242UC243PP)
Mohammed Aman
(1231300581)
Amr Loay Mohamed Kamal
Ahmed(242UC243PS)

How to Compile (Linux)
1) make

How to Run
1) ./server
2) Enter number of players (3-5).
3) In separate terminals run: ./client (one per player)
4) Enter a short name (spaces become underscores).

Game Rules (text-based)
- 3 to 5 players.
- Server rolls the dice (1-6).
- If a player lands on a ladder, they climb up.
- If a player lands on a snake, they slide down.
- Exact roll is required to reach square 100.
- First player to reach square 100 wins.
- Server shows a scoreboard after each game.
- Each player sees the board at their first turn and every 3rd turn after that.

Networking
- TCP IPv4, port 5555.
- Local client uses 127.0.0.1 by default.

Concurrency Model (Hybrid)
- Server forks one child process per client.
- Parent runs two threads: Round Robin scheduler and Logger.
- Shared game state is in POSIX shared memory and protected by process-shared mutexes and semaphores.

Game Flow
- Server waits until the target number of clients connect, then starts the round.
- Turns are scheduled round-robin among connected players.
- After a win, the server announces the winner and scoreboard, then auto-resets for a new round.

Files
- Server.c
- Client.c
- Makefile
- scores.txt (persistent win counts)
- game.log (event log)
