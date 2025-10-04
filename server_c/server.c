#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>   // librerías de sockets en Windows
#include <windows.h>    // para Sleep()

#pragma comment(lib, "ws2_32.lib") // linkear Winsock

#define PORT 8000
#define MAX_CLIENTS 10

SOCKET clients[MAX_CLIENTS];
const char *usernames[MAX_CLIENTS];
int client_Count = 0;

const char *ADMIN_USER = "admin";
const char *ADMIN_PASS = "1234";

DWORD WINAPI client_handler(void *arg) {
    SOCKET clientSock = *(SOCKET*)arg;
    free(arg);

    char buffer[1024];
    int authenticated = 0;
    char username[50] = "anonimo";

    while (1) {
        int bytes = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            printf("Cliente desconectado.\n");
            closesocket(clientSock);
            return 0;
        }
        buffer[bytes] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0; // eliminar saltos de línea

        printf("Recibido: %s\n", buffer);

        // --- Autenticación ---
        if (strncmp(buffer, "AUTH", 4) == 0) {
            char user[50], pass[50];
            sscanf(buffer, "AUTH %s %s", user, pass);
            if (strcmp(user, ADMIN_USER) == 0 && strcmp(pass, ADMIN_PASS) == 0) {
                authenticated = 1;
                strcpy(username, user);
                send(clientSock, "AUTH OK\n", 8, 0);
                printf("Administrador autenticado.\n");
            } else {
                send(clientSock, "AUTH FAIL\n", 10, 0);
                printf("Fallo de autenticacion para usuario %s.\n", user);
            }
        }
        // --- Movimiento del robot ---
        else if (authenticated && strncmp(buffer, "SET", 3) == 0) {
            send(clientSock, "ROBOT MOVED\n", 12, 0);
            printf("Robot movido: %s\n", buffer);
        }
        // --- Lista de usuarios conectados ---
        else if (authenticated && strncmp(buffer, "LIST", 4) == 0) {
            char list[512] = "USERS:\n";
            for (int i = 0; i < client_Count; i++) {
                strcat(list, usernames[i] ? usernames[i] : "anonimo");
                strcat(list, "\n");
            }
            send(clientSock, list, strlen(list), 0);
        }
        // --- Comando desconocido ---
        else {
            send(clientSock, "UNKNOWN COMMAND\n", 17, 0);
        }
    }
    return 0;
}

int main() {
    WSADATA wsa;
    SOCKET serverSock, clientSock;
    struct sockaddr_in serverAddr, clientAddr;
    int clientLen = sizeof(clientAddr);

    // 1. Inicializar Winsock
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Error al inicializar Winsock. Codigo: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Winsock inicializado.\n");

    // 2. Crear socket
    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == INVALID_SOCKET) {
        printf("No se pudo crear socket. Codigo: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    printf("Socket creado.\n");

    // 3. Configurar dirección del servidor
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // 4. Enlazar (bind)
    if (bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Error en bind. Codigo: %d\n", WSAGetLastError());
        closesocket(serverSock);
        WSACleanup();
        return 1;
    }
    printf("Bind hecho en puerto %d.\n", PORT);

    // 5. Escuchar
    if (listen(serverSock, MAX_CLIENTS) == SOCKET_ERROR) {
        printf("Error en listen. Codigo: %d\n", WSAGetLastError());
        closesocket(serverSock);
        WSACleanup();
        return 1;
    }
    printf("Servidor escuchando en puerto %d...\n", PORT);

    // 6. Aceptar múltiples clientes
    while (1) {
        clientSock = accept(serverSock, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSock == INVALID_SOCKET) {
            printf("Error en accept. Codigo: %d\n", WSAGetLastError());
            continue;
        }
        printf("Nuevo Cliente conectado.\n");

        if (client_Count < MAX_CLIENTS) {
            clients[client_Count] = clientSock;
            usernames[client_Count] = "anonimo";
            client_Count++;
        }

        SOCKET *new_sock = (SOCKET*) malloc(sizeof(SOCKET));
        *new_sock = clientSock;
        HANDLE thread = CreateThread(NULL, 0, client_handler, (void*)new_sock, 0, NULL);
        if (thread == NULL) {
            printf("No se pudo crear hilo. error: %d\n", GetLastError());
        } else {
            CloseHandle(thread);
        }
    }

    // 7. Nunca debería llegar aquí
    closesocket(serverSock);
    WSACleanup();
    return 0;
}
