

// 同一进程的多个线程打开相同的文件, 每个线程打印线程号和文件描述符
// 多个线程共享相同的文件描述符表

// gcc thread_fd.c -o thread_fd -lpthread

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

// 线程函数,接收一个整数参数作为线程号
void* thread_function(void* arg) {
    int thread_id = *((int *)arg); // 强制转换void*参数为int*
    const char* filename = "thread_fd.c"; // 所有线程打开相同的文件

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return NULL;
    }

    // 打印线程号和文件描述符
    printf("Thread ID %d - File '%s' opened with FD: %d\n", thread_id, filename, fd);
    sleep(1);
    printf("Thread ID %d - File '%s' closed with FD: %d\n", thread_id, filename, fd);
    close(fd);
    return NULL;
}

int main() {
    pthread_t t1, t2;

    // 创建第一个线程并传递线程号1
    int thread_id_1 = 1;
    if (pthread_create(&t1, NULL, thread_function, &thread_id_1) != 0) {
        perror("Failed to create thread 1");
        return 1;
    }

    // 创建第二个线程并传递线程号2
    int thread_id_2 = 2;
    if (pthread_create(&t2, NULL, thread_function, &thread_id_2) != 0) {
        perror("Failed to create thread 2");
        return 1;
    }

    // 等待线程结束
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}