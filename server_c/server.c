#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8000
#define MAX_CLIENTS 10
#define GRID_SIZE 11

int robot_x = 5, robot_y = 5;

int clients[MAX_CLIENTS];
char *usernames[MAX_CLIENTS];
int client_Count = 0;

pthread_mutex_t lock;

const char *ADMIN_USER = "admin";
const char *ADMIN_PASS = "1234";

// ======================================================
// BROADCAST
// ======================================================
void broadcast_message(const char *msg) {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < client_Count; i++) {
        send(clients[i], msg, strlen(msg), 0);
    }
    pthread_mutex_unlock(&lock);
}

// ======================================================
// REMOVE CLIENT
// ======================================================
void remove_client(int sock) {
    pthread_mutex_lock(&lock);

    int pos = -1;
    for (int i = 0; i < client_Count; i++) {
        if (clients[i] == sock) { pos = i; break; }
    }

    if (pos != -1) {
        close(clients[pos]);
        if (usernames[pos]) free(usernames[pos]);

        for (int j = pos; j < client_Count - 1; j++) {
            clients[j] = clients[j+1];
            usernames[j] = usernames[j+1];
        }
        client_Count--;
    }

    pthread_mutex_unlock(&lock);
}

// ======================================================
// CLIENT THREAD
// ======================================================
void *client_handler(void *arg) {
    int clientSock = *(int *)arg;
    free(arg);

    char buffer[1024];
    int authenticated = 0;
    char username[64] = "anonimo";

    // enviar posición inicial
    char initpos[64];
    sprintf(initpos, "MOVED X=%d Y=%d\n", robot_x, robot_y);
    send(clientSock, initpos, strlen(initpos), 0);

    while (1) {
        int bytes = recv(clientSock, buffer, 1023, 0);
        if (bytes <= 0) {
            printf("Cliente desconectado\n");
            remove_client(clientSock);
            return NULL;
        }

        buffer[bytes] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0;

        printf("Recibido: %s\n", buffer);

        // ---------------- AUTH ----------------
        if (strncmp(buffer, "AUTH", 4) == 0) {
            char user[64], pass[64];
            if (sscanf(buffer, "AUTH %s %s", user, pass) == 2) {
                if (strcmp(user, ADMIN_USER) == 0 && strcmp(pass, ADMIN_PASS) == 0) {
                    authenticated = 1;
                    strcpy(username, user);

                    pthread_mutex_lock(&lock);
                    for (int i = 0; i < client_Count; i++) {
                        if (clients[i] == clientSock) {
                            usernames[i] = strdup(user);
                        }
                    }
                    pthread_mutex_unlock(&lock);

                    send(clientSock, "AUTH OK\n", 8, 0);
                } else {
                    send(clientSock, "AUTH FAIL\n", 10, 0);
                }
            } else {
                send(clientSock, "ERROR formato: AUTH user pass\n", 30, 0);
            }
        }

        // ---------------- SET x y ----------------
        else if (strncmp(buffer, "SET", 3) == 0) {
            int x, y;
            if (sscanf(buffer, "SET %d %d", &x, &y) == 2) {

                if (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE) {

                    pthread_mutex_lock(&lock);
                    robot_x = x;
                    robot_y = y;
                    pthread_mutex_unlock(&lock);

                    char msg[64];
                    sprintf(msg, "MOVED X=%d Y=%d\n", robot_x, robot_y);
                    broadcast_message(msg);

                } else {
                    send(clientSock, "ERROR fuera de limites\n", 23, 0);
                }
            }
            else {
                send(clientSock, "ERROR formato: SET x y\n", 24, 0);
            }
        }

        // ---------------- MOVE ----------------
        else if (strncmp(buffer, "MOVE", 4) == 0) {

            if (!authenticated) {
                send(clientSock, "ERROR AUTH REQUIRED\n", 20, 0);
                continue;
            }

            char dir[32];
            if (sscanf(buffer, "MOVE %s", dir) != 1) {
                send(clientSock, "ERROR formato: MOVE <UP|DOWN|LEFT|RIGHT>\n", 39, 0);
                continue;
            }

            pthread_mutex_lock(&lock);
            int new_x = robot_x;
            int new_y = robot_y;

            if (strcasecmp(dir, "UP") == 0) new_y--;
            else if (strcasecmp(dir, "DOWN") == 0) new_y++;
            else if (strcasecmp(dir, "LEFT") == 0) new_x--;
            else if (strcasecmp(dir, "RIGHT") == 0) new_x++;
            else {
                pthread_mutex_unlock(&lock);
                send(clientSock, "ERROR Unknown MOVE command\n", 27, 0);
                continue;
            }

            if (new_x < 0 || new_x >= GRID_SIZE || new_y < 0 || new_y >= GRID_SIZE) {
                pthread_mutex_unlock(&lock);
                send(clientSock, "BLOCKED\n", 8, 0);
            } else {
                robot_x = new_x;
                robot_y = new_y;
                pthread_mutex_unlock(&lock);

                char msg[64];
                sprintf(msg, "MOVED X=%d Y=%d\n", robot_x, robot_y);
                broadcast_message(msg);
            }
        }

        // ---------------- LIST ----------------
        else if (strncmp(buffer, "LIST", 4) == 0) {

            if (!authenticated) {
                send(clientSock, "ERROR AUTH REQUIRED\n", 20, 0);
                continue;
            }

            char list[1024];
            strcpy(list, "USERS:\n");

            pthread_mutex_lock(&lock);
            for (int i = 0; i < client_Count; i++) {
                strcat(list, usernames[i] ? usernames[i] : "anonimo");
                strcat(list, "\n");
            }
            pthread_mutex_unlock(&lock);

            send(clientSock, list, strlen(list), 0);
        }

        // ---------------- UNKNOWN ----------------
        else {
            send(clientSock, "UNKNOWN COMMAND\n", 17, 0);
        }
    }
}

// ======================================================
// SENSOR THREAD
// ======================================================
void *sensor_thread(void *arg) {
    srand(time(NULL));

    while (1) {
        sleep(15);

        int temp = 20 + rand() % 10;
        int hum = 40 + rand() % 30;
        int pres = 1000 + rand() % 50;

        char msg[128];
        sprintf(msg, "SENSOR TEMP=%d HUM=%d PRES=%d\n", temp, hum, pres);

        broadcast_message(msg);
    }
}

// ======================================================
// MAIN
// ======================================================
int main() {
    pthread_mutex_init(&lock, NULL);

    int serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(serverSock, MAX_CLIENTS) < 0) {
        perror("listen");
        return 1;
    }

    printf("Servidor escuchando en puerto %d...\n", PORT);

    // lanzar hilo de sensores
    pthread_t sen;
    pthread_create(&sen, NULL, sensor_thread, NULL);

    while (1) {
        int clientSock = accept(serverSock, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSock < 0) continue;

        printf("Nuevo cliente conectado.\n");

        pthread_mutex_lock(&lock);
        if (client_Count < MAX_CLIENTS) {
            clients[client_Count] = clientSock;
            usernames[client_Count] = NULL;
            client_Count++;
        }
        pthread_mutex_unlock(&lock);

        int *pclient = malloc(sizeof(int));
        *pclient = clientSock;

        pthread_t th;
        pthread_create(&th, NULL, client_handler, pclient);
        pthread_detach(th);
    }

    return 0;
}
