#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <liburing.h>

#define BUF_SIZE 4096

int main() {
    // 初始化 io_uring
    struct io_uring ring;
    io_uring_queue_init(32, &ring, 0);

    // 打开文件
    int fd = open("Makefile", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    // 分配缓冲区
    char *buf = malloc(BUF_SIZE);
    if (!buf) {
        perror("malloc");
        return 1;
    }

    // 准备提交队列条目(SQE)
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    io_uring_prep_read(sqe, fd, buf, BUF_SIZE, 0);

    // 提交请求
    io_uring_submit(&ring);

    // 等待完成
    struct io_uring_cqe *cqe;
    int ret = io_uring_wait_cqe(&ring, &cqe);
    if (ret < 0) {
        perror("io_uring_wait_cqe");
        return 1;
    }

    // 处理完成事件
    if (cqe->res < 0) {
        fprintf(stderr, "Async read failed: %s\n", strerror(-cqe->res));
    } else {
        printf("Read %d bytes: %.*s\n", cqe->res, cqe->res, buf);
    }

    // 清理
    io_uring_cqe_seen(&ring, cqe);
    io_uring_queue_exit(&ring);
    close(fd);
    free(buf);

    return 0;
}