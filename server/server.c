#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#include "checkers.h"

#define MAX_PLAYERS 16
#define MAX_GAMES 8
#define PORT 1100

typedef struct Player
{
    int socket_fd;
    int id;
    PlayerColor color;
    Game *game;
    int in_game;
} Player;

typedef struct
{
    Game game;
    int in_use;
} GameSlot;

static GameSlot games[MAX_GAMES];

static Player players[MAX_PLAYERS];

static Player *waiting_player = NULL;

pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;

static int send_line(int fd, const char *line)
{
    size_t len = strlen(line);
    ssize_t n = send(fd, line, len, 0);
    return (n == (ssize_t)len) ? 0 : -1;
}

static int recv_line(int fd, char *buf, size_t size)
{
    size_t i = 0;
    while (i + 1 < size)
    {
        char c;
        ssize_t n = recv(fd, &c, 1, 0);
        if (n <= 0)
        {
            return -1;
        }
        if (c == '\n')
        {
            buf[i] = '\0';
            return (int)i;
        }
        buf[i++] = c;
    }
    buf[i] = '\0';
    return (int)i;
}

static int find_game_index(Game *g)
{
    for (int i = 0; i < MAX_GAMES; ++i)
    {
        if (games[i].in_use && &games[i].game == g)
            return i;
    }
    return -1;
}

static Player *find_other_player(Player *p)
{
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        Player *q = &players[i];
        if (q->socket_fd == -1)
            continue;
        if (q == p)
            continue;
        if (q->in_game && q->game == p->game)
            return q;
    }
    return NULL;
}

void *socketThread(void *arg)
{
    int sock = *((int *)arg);
    free(arg);

    printf("New client thread: socket=%d\n", sock);

    Player *me = NULL;

    pthread_mutex_lock(&global_lock);
    int free_index = -1;
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (players[i].socket_fd == -1)
        {
            free_index = i;
            break;
        }
    }

    if (free_index == -1)
    {
        pthread_mutex_unlock(&global_lock);
        send_line(sock, "SERVER_FULL\n");
        close(sock);
        pthread_exit(NULL);
    }

    me = &players[free_index];
    me->socket_fd = sock;
    me->id = free_index + 1;
    me->color = COLOR_WHITE;
    me->game = NULL;
    me->in_game = 0;

    if (waiting_player == NULL)
    {
        waiting_player = me;
        pthread_mutex_unlock(&global_lock);

        send_line(sock, "WAITING_FOR_OPPONENT\n");
    }
    else
    {
        int gindex = -1;
        for (int i = 0; i < MAX_GAMES; ++i)
        {
            if (!games[i].in_use)
            {
                gindex = i;
                break;
            }
        }

        if (gindex == -1)
        {
            pthread_mutex_unlock(&global_lock);
            send_line(sock, "SERVER_NO_MORE_GAMES\n");
            close(sock);
            pthread_exit(NULL);
        }

        games[gindex].in_use = 1;
        Game *g = &games[gindex].game;
        game_init(g);

        Player *p1 = waiting_player;
        Player *p2 = me;
        waiting_player = NULL;

        p1->game = g;
        p2->game = g;
        p1->in_game = 1;
        p2->in_game = 1;

        p1->color = COLOR_WHITE;
        p2->color = COLOR_BLACK;

        pthread_mutex_unlock(&global_lock);

        send_line(p1->socket_fd, "WELCOME WHITE\n");
        send_line(p2->socket_fd, "WELCOME BLACK\n");

        char board_msg[128];
        snprintf(board_msg, sizeof(board_msg), "BOARD ");

        char tmp[64 + 1];
        int idx = 0;
        for (int r = 0; r < BOARD_SIZE; ++r)
        {
            for (int c = 0; c < BOARD_SIZE; ++c)
            {
                tmp[idx++] = g->board[r][c];
            }
        }
        tmp[idx] = '\0';

        strncat(board_msg, tmp, sizeof(board_msg) - strlen(board_msg) - 1);
        strncat(board_msg, "\n", sizeof(board_msg) - strlen(board_msg) - 1);

        send_line(p1->socket_fd, board_msg);
        send_line(p2->socket_fd, board_msg);

        send_line(p1->socket_fd, "YOUR_TURN\n");
        send_line(p2->socket_fd, "OPP_TURN\n");
    }

    char buf[256];

    while (1)
    {
        int n = recv_line(sock, buf, sizeof(buf));
        if (n <= 0)
        {
            printf("Client %d disconnected\n", me->id);
            pthread_mutex_lock(&global_lock);

            if (me->in_game && me->game != NULL)
            {
                Game *g = me->game; // запомним, пока не обнулили
                Player *op = find_other_player(me);
                if (op != NULL)
                {
                    send_line(op->socket_fd, "OPPONENT_LEFT\n");
                    op->in_game = 0;
                    op->game = NULL;
                }
                me->in_game = 0;
                me->game = NULL;

                int gi = find_game_index(g);
                if (gi >= 0)
                {
                    games[gi].in_use = 0;
                }
            }
            else if (waiting_player == me)
            {
                waiting_player = NULL;
            }

            me->socket_fd = -1;
            pthread_mutex_unlock(&global_lock);
            break;
        }

        printf("Client %d sent: %s\n", me->id, buf);

        if (strcmp(buf, "QUIT") == 0)
        {
            pthread_mutex_lock(&global_lock);

            if (me->in_game && me->game != NULL)
            {
                Game *g = me->game;
                Player *op = find_other_player(me);
                if (op != NULL)
                {
                    send_line(op->socket_fd, "OPPONENT_LEFT\n");
                    op->in_game = 0;
                    op->game = NULL;
                }
                me->in_game = 0;
                me->game = NULL;

                int gi = find_game_index(g);
                if (gi >= 0)
                {
                    games[gi].in_use = 0;
                }
            }
            else if (waiting_player == me)
            {
                waiting_player = NULL;
            }

            me->socket_fd = -1;
            pthread_mutex_unlock(&global_lock);
            break;
        }

        if (strncmp(buf, "MOVE", 4) == 0)
        {
            int fr, fc, tr, tc;
            if (sscanf(buf, "MOVE %d %d %d %d", &fr, &fc, &tr, &tc) != 4)
            {
                send_line(sock, "ERROR_BAD_FORMAT\n");
                send_line(sock, "YOUR_TURN\n");
                continue;
            }

            pthread_mutex_lock(&global_lock);

            if (!me->in_game || me->game == NULL)
            {
                pthread_mutex_unlock(&global_lock);
                send_line(sock, "ERROR_NOT_IN_GAME\n");
                continue;
            }

            Game *g = me->game;
            Player *op = find_other_player(me);

            if (g->turn != me->color && !g->must_continue_capture)
            {
                pthread_mutex_unlock(&global_lock);
                send_line(sock, "ERROR_NOT_YOUR_TURN\n");
                continue;
            }

            if (!game_apply_move(g, fr, fc, tr, tc))
            {
                int must = g->must_continue_capture;
                pthread_mutex_unlock(&global_lock);

                send_line(sock, "MOVE_INVALID\n");
                if (must)
                    send_line(sock, "YOUR_TURN_CONTINUE_CAPTURE\n");
                else
                    send_line(sock, "YOUR_TURN\n");
                continue;
            }

            send_line(sock, "MOVE_OK\n");
            if (op != NULL)
            {
                send_line(op->socket_fd, "OPPONENT_MOVED\n");
            }

            char board_msg[128];
            snprintf(board_msg, sizeof(board_msg), "BOARD ");

            char tmp[64 + 1];
            int idx = 0;
            for (int r = 0; r < BOARD_SIZE; ++r)
            {
                for (int c = 0; c < BOARD_SIZE; ++c)
                {
                    tmp[idx++] = g->board[r][c];
                }
            }
            tmp[idx] = '\0';
            strncat(board_msg, tmp, sizeof(board_msg) - strlen(board_msg) - 1);
            strncat(board_msg, "\n", sizeof(board_msg) - strlen(board_msg) - 1);

            send_line(sock, board_msg);
            if (op != NULL)
            {
                send_line(op->socket_fd, board_msg);
            }

            if (game_is_finished(g))
            {
                if (g->result == GAME_WHITE_WIN)
                {
                    if (me->color == COLOR_WHITE)
                    {
                        send_line(sock, "YOU_WIN\n");
                        if (op)
                            send_line(op->socket_fd, "YOU_LOSE\n");
                    }
                    else
                    {
                        send_line(sock, "YOU_LOSE\n");
                        if (op)
                            send_line(op->socket_fd, "YOU_WIN\n");
                    }
                }
                else if (g->result == GAME_BLACK_WIN)
                {
                    if (me->color == COLOR_BLACK)
                    {
                        send_line(sock, "YOU_WIN\n");
                        if (op)
                            send_line(op->socket_fd, "YOU_LOSE\n");
                    }
                    else
                    {
                        send_line(sock, "YOU_LOSE\n");
                        if (op)
                            send_line(op->socket_fd, "YOU_WIN\n");
                    }
                }
                else
                {
                    send_line(sock, "DRAW\n");
                    if (op)
                        send_line(op->socket_fd, "DRAW\n");
                }

                if (op)
                {
                    op->in_game = 0;
                    op->game = NULL;
                }
                me->in_game = 0;
                me->game = NULL;

                int gi = find_game_index(g);
                if (gi >= 0)
                {
                    games[gi].in_use = 0;
                }
            }
            else
            {
                if (g->must_continue_capture)
                {
                    send_line(sock, "YOUR_TURN_CONTINUE_CAPTURE\n");
                    if (op)
                        send_line(op->socket_fd, "OPP_TURN_CAPTURE_CHAIN\n");
                }
                else
                {
                    if (op)
                    {
                        send_line(op->socket_fd, "YOUR_TURN\n");
                    }
                    send_line(sock, "OPP_TURN\n");
                }
            }

            pthread_mutex_unlock(&global_lock);
        }
        else
        {
            send_line(sock, "ERROR_UNKNOWN_COMMAND\n");
        }
    }

    close(sock);
    pthread_exit(NULL);
}

int main()
{
    int serverSocket;
    struct sockaddr_in serverAddr;
    struct sockaddr_storage serverStorage;
    socklen_t addr_size;

    serverSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("bind");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, 50) == 0)
        printf("Listening on port %d\n", PORT);
    else
    {
        perror("listen");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        players[i].socket_fd = -1;
        players[i].in_game = 0;
        players[i].game = NULL;
        players[i].id = i + 1;
    }
    for (int i = 0; i < MAX_GAMES; ++i)
    {
        games[i].in_use = 0;
    }

    while (1)
    {
        addr_size = sizeof serverStorage;
        int *newSocket = malloc(sizeof(int));
        if (!newSocket)
        {
            perror("malloc");
            continue;
        }

        *newSocket = accept(serverSocket, (struct sockaddr *)&serverStorage, &addr_size);
        if (*newSocket < 0)
        {
            perror("accept");
            free(newSocket);
            continue;
        }

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, socketThread, newSocket) != 0)
        {
            perror("pthread_create");
            close(*newSocket);
            free(newSocket);
            continue;
        }

        pthread_detach(thread_id);
    }

    close(serverSocket);
    return 0;
}
