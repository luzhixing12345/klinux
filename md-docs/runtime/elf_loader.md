
# elf_loader

```c
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s \n", argv[0]);
        return 1;
    }

    // Open the binary file
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    // Get the file size
    off_t file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    // Allocate memory for the binary
    void *mem = mmap(NULL, file_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE, fd, 0);
    if (mem == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    // Close the file
    close(fd);

    // Cast the memory address to a function pointer and call it
    void (*binary_func)() = (void (*)())mem;
    binary_func();

    // Clean up
    munmap(mem, file_size);

    return 0;
}
```

> /usr/src/linux/fs/binfmt_elf.c