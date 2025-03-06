
// open + dup

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
    
    int fd1, fd2;
    fd1 = open("test.txt", O_WRONLY | O_CREAT, 0644);
    fd2 = dup(fd1);
    printf("fd1 = %d, fd2 = %d\n", fd1, fd2);

    int seek_offset = 10;
    lseek(fd1, seek_offset, SEEK_SET);
    printf("seek fd1[%d] offset %d\n", fd1, seek_offset);
    printf("fd2 tell = %ld\n", lseek(fd2, 0, SEEK_CUR));
    return 0;
}