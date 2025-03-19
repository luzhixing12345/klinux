
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>

#define SERVER_PORT 8080
#define MAX_PENDING_CONNECTIONS 5
#define BUFFER_SIZE 80

int main() {
    struct sockaddr_in server_addr, client_addr;
    int server_socket, client_socket;
    socklen_t client_addr_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_PENDING_CONNECTIONS) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", SERVER_PORT);

    char buf[BUFFER_SIZE];
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }
        printf("Connection accepted from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        int n = read(client_socket, buf, BUFFER_SIZE);
        for (int i=0;i<n;i++) {
            buf[i] = toupper(buf[i]);
        }
        write(client_socket, buf, n);
        close(client_socket);
        // break;
    }

    close(server_socket);

    return 0;
}