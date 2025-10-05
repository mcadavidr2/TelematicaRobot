#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8000
#define MAX_CLIENTS 10
#define GRID_SIZE 11

// estado global
SOCKET clients[MAX_CLIENTS];
char *usernames[MAX_CLIENTS];
int client_Count = 0;

int robot_x = 5;
int robot_y = 5;

const char *ADMIN_USER = "admin";
const char *ADMIN_PASS = "1234";

// sincronización
CRITICAL_SECTION cs;

// Helpers
void broadcast_message(const char *msg) {
    EnterCriticalSection(&cs);
    for (int i = 0; i < client_Count; i++) {
        int ret = send(clients[i], msg, (int)strlen(msg), 0);
        if (ret == SOCKET_ERROR) {
            printf("  send error idx=%d sock=%llu err=%d\n", i, (unsigned long long)clients[i], WSAGetLastError());
        } else {
            printf("  send-> idx=%d sock=%llu ret=%d\n", i, (unsigned long long)clients[i], ret);
        }
    }
    LeaveCriticalSection(&cs);
}

void remove_client_socket(SOCKET s) {
    EnterCriticalSection(&cs);
    int found = -1;
    for (int i = 0; i < client_Count; i++) {
        if (clients[i] == s) {
            found = i;
            break;
        }
    }
    if (found != -1) {
        closesocket(clients[found]);
        // free username if allocated
        if (usernames[found]) {
            free(usernames[found]);
            usernames[found] = NULL;
        }
        // shift
        for (int j = found; j < client_Count - 1; j++) {
            clients[j] = clients[j + 1];
            usernames[j] = usernames[j + 1];
        }
        clients[client_Count - 1] = 0;
        usernames[client_Count - 1] = NULL;
        client_Count--;
        printf("Cliente removido. client_Count=%d\n", client_Count);
    }
    LeaveCriticalSection(&cs);
}

DWORD WINAPI client_handler(void *arg) {
    SOCKET clientSock = *(SOCKET*)arg;
    free(arg);

    char buffer[1024];
    int authenticated = 0;
    char username[64] = "anonimo";

    // send initial position to the new client (optional)
    {
        char tmp[64];
        sprintf(tmp, "MOVED X=%d Y=%d\n", robot_x, robot_y);
        send(clientSock, tmp, (int)strlen(tmp), 0);
    }

    while (1) {
        int bytes = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            printf("Cliente desconectado (sock=%llu). bytes=%d\n", (unsigned long long)clientSock, bytes);
            remove_client_socket(clientSock);
            return 0;
        }

        buffer[bytes] = '\0';
        // quitar CR/LF
        buffer[strcspn(buffer, "\r\n")] = 0;

        printf("Recibido (sock=%llu): %s\n", (unsigned long long)clientSock, buffer);

        // --- AUTH ---
        if (strncmp(buffer, "AUTH", 4) == 0) {
            char user[64], pass[64];
            if (sscanf(buffer, "AUTH %63s %63s", user, pass) == 2) {
                if (strcmp(user, ADMIN_USER) == 0 && strcmp(pass, ADMIN_PASS) == 0) {
                    authenticated = 1;
                    strncpy(username, user, sizeof(username)-1);
                    // mark username in clients array
                    EnterCriticalSection(&cs);
                    for (int i = 0; i < client_Count; i++) {
                        if (clients[i] == clientSock) {
                            // replace username (free previous if any)
                            if (usernames[i]) free(usernames[i]);
                            usernames[i] = _strdup(user);
                            break;
                        }
                    }
                    LeaveCriticalSection(&cs);

                    send(clientSock, "AUTH OK\n", 8, 0);
                    printf("Administrador autenticado (sock=%llu).\n", (unsigned long long)clientSock);
                } else {
                    send(clientSock, "AUTH FAIL\n", 10, 0);
                    printf("Fallo de autenticacion para usuario %s (sock=%llu).\n", user, (unsigned long long)clientSock);
                }
            } else {
                send(clientSock, "ERROR formato: AUTH user pass\n", 30, 0);
            }
        }

        // --- SET x y (absoluto) ---
        else if (strncmp(buffer, "SET", 3) == 0) {
            int x, y;
            if (sscanf(buffer, "SET %d %d", &x, &y) == 2) {
                if (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE) {
                    EnterCriticalSection(&cs);
                    robot_x = x;
                    robot_y = y;
                    LeaveCriticalSection(&cs);

                    char msg[64];
                    sprintf(msg, "MOVED X=%d Y=%d\n", robot_x, robot_y);
                    printf("DEBUG: Enviando -> %s", msg);
                    broadcast_message(msg);
                } else {
                    send(clientSock, "ERROR fuera de limites\n", 23, 0);
                }
            } else {
                send(clientSock, "ERROR formato: SET x y\n", 24, 0);
            }
        }

        // --- MOVE direction ---
        else if (strncmp(buffer, "MOVE", 4) == 0) {
            // require authentication to move
            if (!authenticated) {
                send(clientSock, "ERROR AUTH REQUIRED\n", 20, 0);
                continue;
            }

            // parse direction token
            char dir[32] = {0};
            if (sscanf(buffer, "MOVE %31s", dir) != 1) {
                send(clientSock, "ERROR formato: MOVE <UP|DOWN|LEFT|RIGHT>\n", 39, 0);
                continue;
            }

            int new_x, new_y;
            EnterCriticalSection(&cs);
            new_x = robot_x;
            new_y = robot_y;
            if (_stricmp(dir, "UP") == 0) {
                new_y = robot_y - 1;
            } else if (_stricmp(dir, "DOWN") == 0) {
                new_y = robot_y + 1;
            } else if (_stricmp(dir, "LEFT") == 0) {
                new_x = robot_x - 1;
            } else if (_stricmp(dir, "RIGHT") == 0) {
                new_x = robot_x + 1;
            } else {
                LeaveCriticalSection(&cs);
                send(clientSock, "ERROR Unknown MOVE command\n", 27, 0);
                continue;
            }

            // validate
            if (new_x < 0 || new_x >= GRID_SIZE || new_y < 0 || new_y >= GRID_SIZE) {
                LeaveCriticalSection(&cs);
                send(clientSock, "BLOCKED\n", 8, 0);
                printf("Movimiento bloqueado desde sock=%llu dir=%s\n", (unsigned long long)clientSock, dir);
            } else {
                robot_x = new_x;
                robot_y = new_y;
                LeaveCriticalSection(&cs);

                char msg[64];
                sprintf(msg, "MOVED X=%d Y=%d\n", robot_x, robot_y);
                printf("DEBUG: Enviando -> %s", msg);
                broadcast_message(msg);
            }
        }

        // --- LIST ---
        else if (strncmp(buffer, "LIST", 4) == 0) {
            // require authentication to list
            if (!authenticated) {
                send(clientSock, "ERROR AUTH REQUIRED\n", 20, 0);
                continue;
            }
            char list[1024];
            strcpy(list, "USERS:\n");
            EnterCriticalSection(&cs);
            for (int i = 0; i < client_Count; i++) {
                if (usernames[i]) {
                    strcat(list, usernames[i]);
                } else {
                    strcat(list, "anonimo");
                }
                strcat(list, "\n");
            }
            LeaveCriticalSection(&cs);
            send(clientSock, list, (int)strlen(list), 0);
        }

        // --- unknown ---
        else {
            send(clientSock, "UNKNOWN COMMAND\n", 17, 0);
        }
    }

    return 0;
}
DWORD WINAPI sensor_thread(void *arg) {
    srand((unsigned)time(NULL));
    while (1) {
        Sleep(15000); // cada 15 segundos

        int temp = 20 + rand() % 10;     // 20-29
        int hum = 40 + rand() % 30;      // 40-69
        int pres = 1000 + rand() % 50;   // 1000-1049

        char msg[128];
        sprintf(msg, "SENSOR TEMP=%d HUM=%d PRES=%d\n", temp, hum, pres);

        printf("DEBUG: Generando %s", msg);

        // Enviar a todos los clientes
        for (int i = 0; i < client_Count; i++) {
            send(clients[i], msg, (int)strlen(msg), 0);
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    WSADATA wsa;
    SOCKET serverSock, clientSock;
    struct sockaddr_in serverAddr, clientAddr;
    int clientLen = sizeof(clientAddr);

    InitializeCriticalSection(&cs);

    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Error al inicializar Winsock. Codigo: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Winsock inicializado.\n");

    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == INVALID_SOCKET) {
        printf("No se pudo crear socket. Codigo: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    printf("Socket creado.\n");

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Error en bind. Codigo: %d\n", WSAGetLastError());
        closesocket(serverSock);
        WSACleanup();
        return 1;
    }
    printf("Bind hecho en puerto %d.\n", PORT);

    if (listen(serverSock, MAX_CLIENTS) == SOCKET_ERROR) {
        printf("Error en listen. Codigo: %d\n", WSAGetLastError());
        closesocket(serverSock);
        WSACleanup();
        return 1;
    }
    printf("Servidor escuchando en puerto %d...\n", PORT);

    // Lanzar hilo de sensores
HANDLE sensorHandle = CreateThread(NULL, 0, sensor_thread, NULL, 0, NULL);
if (sensorHandle == NULL) {
    printf("No se pudo crear hilo de sensores. error: %d\n", GetLastError());
} else {
    CloseHandle(sensorHandle);
}


    while (1) {
        clientSock = accept(serverSock, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSock == INVALID_SOCKET) {
            printf("Error en accept. Codigo: %d\n", WSAGetLastError());
            continue;
        }
        printf("Nuevo Cliente conectado. sock=%llu addr=%s:%d\n",
               (unsigned long long)clientSock,
               inet_ntoa(clientAddr.sin_addr),
               ntohs(clientAddr.sin_port));

        EnterCriticalSection(&cs);
        if (client_Count < MAX_CLIENTS) {
            clients[client_Count] = clientSock;
            usernames[client_Count] = NULL; // anonimo por defecto
            client_Count++;
            printf("  client_Count=%d\n", client_Count);
        } else {
            // servidor lleno: rechazar
            send(clientSock, "ERROR server full\n", 18, 0);
            closesocket(clientSock);
            LeaveCriticalSection(&cs);
            continue;
        }
        LeaveCriticalSection(&cs);

        SOCKET *new_sock = (SOCKET*) malloc(sizeof(SOCKET));
        *new_sock = clientSock;
        HANDLE thread = CreateThread(NULL, 0, client_handler, (void*)new_sock, 0, NULL);
        if (thread == NULL) {
            printf("No se pudo crear hilo. error: %d\n", GetLastError());
            remove_client_socket(clientSock);
            free(new_sock);
        } else {
            CloseHandle(thread);
        }
    }

    // cleanup (nunca se llega aquí normalmente)
    DeleteCriticalSection(&cs);
    closesocket(serverSock);
    WSACleanup();
    return 0;
}
