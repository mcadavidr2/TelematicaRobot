#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8000

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // 1. Crear socket TCP
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Error creando socket");
        exit(1);
    }

    // 2. Configurar dirección
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; 
    server_addr.sin_port = htons(PORT);

    // 3. Enlazar
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error en bind");
        exit(1);
    }

    // 4. Escuchar
    listen(server_sock, 1);
    printf("Servidor escuchando en puerto %d...\n", PORT);

    // 5. Aceptar un cliente
    client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
    if (client_sock < 0) {
        perror("Error en accept");
        exit(1);
    }
    printf("Cliente conectado: %s:%d\n",
           inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port));

    // 6. Enviar mensajes cada 5s
    int x = 0, y = 0;
    while (1) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "MOVED X=%d Y=%d\n", x, y);
        send(client_sock, buffer, strlen(buffer), 0);
        printf("Enviado: %s", buffer);

        // mover coords un poco
        x = (x + 1) % 11;
        y = (y + 1) % 11;

        sleep(5); // esperar 5 segundos
    }

    // nunca llegamos aquí en este ejemplo
    close(client_sock);
    close(server_sock);
    return 0;
}