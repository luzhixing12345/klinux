#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <string.h>
#include <errno.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define TIMEOUT 5000  // 超时时间5秒

int main() {
    int listen_fd, conn_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    struct pollfd fds[2];
    int nfds = 2;

    // 创建监听套接字
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 设置服务器地址信息
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 绑定和监听
    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }
    if (listen(listen_fd, 5) < 0) {
        perror("listen");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", PORT);

    // 初始化 pollfd 结构数组
    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;  // 监听可读事件

    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN;  // 监听标准输入可读事件

    while (1) {
        int ret = poll(fds, nfds, TIMEOUT);
        if (ret == -1) {
            perror("poll");
            exit(EXIT_FAILURE);
        } else if (ret == 0) {
            printf("Timeout occurred! No data after %d milliseconds.\n", TIMEOUT);
            continue;
        }

        // 检查监听套接字是否有新连接
        if (fds[0].revents & POLLIN) {
            if ((conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len)) < 0) {
                perror("accept");
                continue;
            }
            printf("New connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            char buffer[BUFFER_SIZE];
            ssize_t num_bytes = read(conn_fd, buffer, sizeof(buffer) - 1);
            if (num_bytes > 0) {
                buffer[num_bytes] = '\0';
                printf("Input: %s", buffer);
            }
            close(conn_fd);  // 这里只是演示,关闭连接
        }

        // 检查标准输入是否有数据可读
        if (fds[1].revents & POLLIN) {
            ssize_t num_bytes = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
            if (num_bytes > 0) {
                buffer[num_bytes] = '\0';
                printf("Input: %s", buffer);
            }
        }
    }

    close(listen_fd);
    return 0;
}