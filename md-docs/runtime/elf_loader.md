
# elf_loader

linux环境下我们启动一个程序一般都是通过shell拉起来的.或者通过一个程序调用exec系列函数进行进程替换的

```bash
grep -rnw . -e 'register_binfmt' --include \*.c --include \*.h
```

![20240621154352](https://raw.githubusercontent.com/learner-lu/picbed/master/20240621154352.png)

> 6.6 相比于 4.4.6 删除了对于 em86/a.out 格式的支持

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

## 参考

- [进程装载过程分析(execve系统调用分析)](https://www.cnblogs.com/tjyuanxi/p/9313253.html)