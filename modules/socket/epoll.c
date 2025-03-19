#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#define PORT 8080
#define MAX_EVENTS 10
#define BUFFER_SIZE 1024

int main() {
    int listen_fd, conn_fd, epoll_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    struct epoll_event ev, events[MAX_EVENTS];
    int nfds;

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

    // 创建 epoll 实例
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    // 添加监听套接字到 epoll 实例
    ev.events = EPOLLIN;  // 监听可读事件
    ev.data.fd = listen_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) == -1) {
        perror("epoll_ctl: listen_fd");
        close(listen_fd);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    // 添加标准输入到 epoll 实例
    ev.events = EPOLLIN;  // 监听可读事件
    ev.data.fd = STDIN_FILENO;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) == -1) {
        perror("epoll_ctl: stdin");
        close(listen_fd);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    while (1) {
        nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            close(listen_fd);
            close(epoll_fd);
            exit(EXIT_FAILURE);
        }

        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == listen_fd) {
                // 处理新连接
                conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
                if (conn_fd == -1) {
                    perror("accept");
                    continue;
                }
                printf("New connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                ssize_t num_bytes = read(conn_fd, buffer, sizeof(buffer) - 1);
                if (num_bytes > 0) {
                    buffer[num_bytes] = '\0';
                    printf("Get: %s", buffer);
                }

                // 添加新连接到 epoll 实例
                ev.events = EPOLLIN;
                ev.data.fd = conn_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &ev) == -1) {
                    perror("epoll_ctl: conn_fd");
                    close(conn_fd);
                }
            } else if (events[n].data.fd == STDIN_FILENO) {
                // 处理标准输入
                ssize_t num_bytes = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
                if (num_bytes > 0) {
                    buffer[num_bytes] = '\0';
                    printf("Input: %s", buffer);
                }
            } else {
                // 处理其他已连接套接字的数据
                ssize_t num_bytes = read(events[n].data.fd, buffer, sizeof(buffer) - 1);
                if (num_bytes <= 0) {
                    if (num_bytes == 0) {
                        printf("Connection closed\n");
                    } else {
                        perror("read");
                    }
                    close(events[n].data.fd);
                } else {
                    buffer[num_bytes] = '\0';
                    printf("Received data: %s", buffer);
                }
            }
        }
    }

    close(listen_fd);
    close(epoll_fd);
    return 0;
}