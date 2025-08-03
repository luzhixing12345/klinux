
// 连续打开同一个文件两次

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
    
    int fd1, fd2;
    fd1 = open("test.txt", O_WRONLY | O_CREAT, 0644);
    fd2 = open("test.txt", O_WRONLY | O_CREAT, 0644);
    lseek(fd1, 3, SEEK_SET);
    write(fd1, "123", 3);
    write(fd2, "456", 3);
    close(fd1);
    close(fd2);
    // cat test.txt
    // 456123
    return 0;
}

// int main(int argc, char **argv) {
    
//     int fd1, fd2;
//     fd1 = open("test.txt", O_WRONLY | O_CREAT, 0644);
//     fd2 = open("test.txt", O_WRONLY | O_CREAT, 0644);
//     write(fd1, "123\n", 4);
//     write(fd2, "456\n", 4);
//     close(fd1);
//     close(fd2);
//     // cat test.txt
//     // 456
//     return 0;
// }