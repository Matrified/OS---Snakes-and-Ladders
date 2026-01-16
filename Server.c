#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <time.h>

#define PORT 5555
#define MAX_PLAYERS 3
#define SHM_NAME "/sll_shm"

typedef struct {
    int positions[MAX_PLAYERS];
    int active[MAX_PLAYERS];
    int current_turn;
    int game_over;

    pthread_mutex_t game_mutex;
    pthread_mutex_t log_mutex;
} shared_game;

shared_game *game;
FILE *log_file;

/* ---------------- LOGGER ---------------- */

void log_event(const char *msg) {
    pthread_mutex_lock(&game->log_mutex);
    fprintf(log_file, "%s\n", msg);
    fflush(log_file);
    pthread_mutex_unlock(&game->log_mutex);
}

/* ---------------- SCHEDULER THREAD ---------------- */

void *scheduler(void *arg) {
    while (1) {
        sleep(1);

        pthread_mutex_lock(&game->game_mutex);

        if (game->game_over) {
            pthread_mutex_unlock(&game->game_mutex);
            continue;
        }

        int next = (game->current_turn + 1) % MAX_PLAYERS;

        while (!game->active[next]) {
            next = (next + 1) % MAX_PLAYERS;
        }

        game->current_turn = next;

        pthread_mutex_unlock(&game->game_mutex);
    }
}

/* ---------------- GAME LOGIC ---------------- */

int roll_dice() {
    return (rand() % 6) + 1;
}

void handle_client(int sock, int id) {
    char buffer[256];

    while (1) {
        pthread_mutex_lock(&game->game_mutex);

        if (game->game_over) {
            pthread_mutex_unlock(&game->game_mutex);
            write(sock, "Game Over\n", 10);
            break;
        }

        if (game->current_turn != id) {
            pthread_mutex_unlock(&game->game_mutex);
            sleep(1);
            continue;
        }

        pthread_mutex_unlock(&game->game_mutex);

        write(sock, "Your turn. Press ENTER to roll.\n", 32);
        read(sock, buffer, sizeof(buffer));

        int dice = roll_dice();

        pthread_mutex_lock(&game->game_mutex);
        game->positions[id] += dice;

        char logmsg[256];
        sprintf(logmsg, "Player %d rolled %d (pos=%d)",
                id, dice, game->positions[id]);
        log_event(logmsg);

        if (game->positions[id] >= 100) {
            game->game_over = 1;
            sprintf(logmsg, "Player %d WON", id);
            log_event(logmsg);
            write(sock, "YOU WIN!\n", 9);
        }

        pthread_mutex_unlock(&game->game_mutex);
        sleep(1);
    }

    close(sock);
    exit(0);
}

/* ---------------- SIGNAL HANDLER ---------------- */

void reap(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

/* ---------------- MAIN ---------------- */

int main() {
    srand(time(NULL));
    signal(SIGCHLD, reap);

    log_file = fopen("game.log", "a");

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(shared_game));
    game = mmap(NULL, sizeof(shared_game),
                PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    memset(game, 0, sizeof(shared_game));

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&game->game_mutex, &attr);
    pthread_mutex_init(&game->log_mutex, &attr);

    for (int i = 0; i < MAX_PLAYERS; i++)
        game->active[i] = 1;

    pthread_t sched_thread;
    pthread_create(&sched_thread, NULL, scheduler, NULL);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, MAX_PLAYERS);

    printf("Server running on port %d\n", PORT);

    for (int i = 0; i < MAX_PLAYERS; i++) {
        int client_fd = accept(server_fd, NULL, NULL);

        pid_t pid = fork();
        if (pid == 0) {
            handle_client(client_fd, i);
        }
    }

    pause();
    return 0;
}

