
// open + fork

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    int fd = open("test.txt", O_WRONLY | O_CREAT, 0644);
    if (fork() == 0) {
        write(fd, "123\n", 4);
        close(fd);
    } else {
        write(fd, "456\n", 4);
        wait(NULL);
        close(fd);
    }
    // cat test.txt
    // 456
    // 123
    return 0;
}