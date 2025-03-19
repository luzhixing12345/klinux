#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int listen_fd, conn_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    fd_set readfds, tempfds;
    int max_fd;

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

    // 初始化文件描述符集合
    FD_ZERO(&readfds);
    FD_SET(listen_fd, &readfds);
    max_fd = listen_fd;

    while (1) {
        tempfds = readfds;

        // 使用 select 进行 I/O 多路复用
        if (select(max_fd + 1, &tempfds, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // 检查监听套接字是否有新连接
        if (FD_ISSET(listen_fd, &tempfds)) {
            if ((conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len)) < 0) {
                perror("accept");
                continue;
            }
            printf("New connection from %s:%d\n",
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            FD_SET(conn_fd, &readfds);
            if (conn_fd > max_fd) {
                max_fd = conn_fd;
            }
        }

        // 检查已连接套接字是否有数据可读
        for (int i = listen_fd + 1; i <= max_fd; i++) {
            if (FD_ISSET(i, &tempfds)) {
                ssize_t num_bytes = read(i, buffer, sizeof(buffer) - 1);
                if (num_bytes <= 0) {
                    // 连接关闭或出错
                    if (num_bytes == 0) {
                        printf("Connection closed by peer\n");
                    } else {
                        perror("read");
                    }
                    close(i);
                    FD_CLR(i, &readfds);
                } else {
                    buffer[num_bytes] = '\0'; // 确保缓冲区以空字符结尾
                    printf("Received data: %s", buffer);
                }
            }
        }
    }

    close(listen_fd);
    return 0;
}