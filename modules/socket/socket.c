#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_PENDING_CONNECTIONS 5
#define BUFFER_SIZE 1024

int main() {
    char buffer[BUFFER_SIZE];
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // 创建套接字
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 设置服务器地址结构
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 绑定套接字
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // 监听套接字
    if (listen(server_socket, MAX_PENDING_CONNECTIONS) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // 接受连接并处理
    while (1) {
        // 等待客户端连接
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        // 处理连接,这里简单打印客户端的信息
        printf("Connection accepted from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // 在这里可以进行进一步的通信处理,读取/写入数据等
        int num_bytes = recvfrom(client_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_addr_len);
        buffer[num_bytes] = '\0';
        printf("Received %d bytes from client: %s\n", num_bytes, buffer);

        // 关闭客户端套接字
        close(client_socket);
    }

    // 关闭服务器套接字
    close(server_socket);

    return 0;
}