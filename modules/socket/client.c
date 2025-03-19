
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_PORT 8080
#define BUFFER_SIZE 80

int main(int argc, char **argv) {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }

    char buf[BUFFER_SIZE];
    printf("Enter message: ");
    fgets(buf, sizeof(buf), stdin);

    write(client_socket, buf, strlen(buf));

    read(client_socket, buf, BUFFER_SIZE);
    printf("response from server: %s\n", buf);
    
    return 0;
}