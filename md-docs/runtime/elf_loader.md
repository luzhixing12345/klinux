
# elf_loader

linux环境下我们启动一个程序一般都是通过shell拉起来的.或者通过一个程序调用exec系列函数进行进程替换的

```bash
grep -rnw . -e 'register_binfmt' --include \*.c --include \*.h
```

![20240621154352](https://raw.githubusercontent.com/learner-lu/picbed/master/20240621154352.png)

> 6.6 相比于 4.4.6 删除了对于 em86/a.out 格式的支持

`execve` 系统调用的函数原型如下:

```c
int execve(const char *filename, char *const argv[], char *const envp[]);
```

参数说明:
- `filename`: 要执行的可执行文件的路径.
- `argv`: 传递给新程序的参数列表(通常是命令行参数).
- `envp`: 传递给新程序的环境变量列表.

当用户进程调用 `execve` 时,系统会从用户态切换到内核态,并进入内核的相应处理函数.这个处理过程大致可以分为以下几步:

1. **用户态调用**: 用户进程调用 `execve` 函数.
   
2. **系统调用入口**: 系统调用通过中断或快速系统调用路径进入内核,定位到 `sys_execve` 函数.  
   在 Linux 源码中,`sys_execve` 的定义可以在 `fs/exec.c` 文件中找到:

   ```c
   SYSCALL_DEFINE3(execve,
                   const char __user *, filename,
                   const char __user *const __user *, argv,
                   const char __user *const __user *, envp)
   {
       return do_execve(getname(filename), argv, envp);
   }
   ```

   该函数的核心是调用 `do_execve` 来处理实际的进程装载.

`do_execve` 是核心函数,它主要负责处理用户传入的参数、进行安全检查以及最终的进程替换.源码如下:

```c
int do_execve(struct filename *filename,
              const char __user *const __user *__argv,
              const char __user *const __user *__envp)
{
    struct linux_binprm *bprm;
    int retval;

    // 创建二进制参数结构体,包含了关于可执行文件的信息
    bprm = create_binprm(filename);
    if (IS_ERR(bprm))
        return PTR_ERR(bprm);

    // 将用户提供的 argv 和 envp 复制到内核空间
    retval = copy_strings(bprm, __argv, __envp);
    if (retval < 0)
        goto out;

    // 加载可执行文件
    retval = search_binary_handler(bprm);
    if (retval >= 0) {
        return retval;  // 执行成功
    }

out:
    free_bprm(bprm);
    return retval;
}
```

#### 1. `create_binprm`

`create_binprm` 函数负责创建 `linux_binprm` 结构体,这是一个保存可执行文件信息的关键数据结构.它包含了文件描述符、参数信息、文件路径等.

#### 2. `copy_strings`

`copy_strings` 函数将用户空间的 `argv` 和 `envp` 参数复制到内核空间,这一步非常重要,因为内核在执行可执行文件时需要完全控制参数内容.

#### 3. `search_binary_handler`

`search_binary_handler` 是进程装载的核心函数,它负责找到合适的二进制处理程序(比如 ELF 加载器)并调用它来装载可执行文件.以下是一个简化的流程:

1. **检查 ELF 文件头**:通过读取文件头判断它是否为 ELF 格式.
   
2. **设置进程内存布局**:包括为代码段、数据段、堆和栈分配内存.

3. **加载程序头表**:读取可执行文件的程序头表,并根据其中的信息将相应的段(如 `.text`、`.data` 等)加载到内存.

4. **设置入口点**:根据 ELF 文件的入口点信息设置 CPU 的程序计数器(PC),以便执行从该入口点开始的代码.

5. **处理动态链接**(如适用):如果是动态链接的 ELF 文件,动态链接器(`ld.so`)会被加载并负责加载共享库.

### 四、进程替换和上下文切换

当 `search_binary_handler` 成功执行后,旧进程的内存空间会被新进程的内存空间替换.以下是关键步骤:

1. **清理旧进程的内存**:释放旧进程的内存,包括堆栈、堆、共享库等.
   
2. **加载新进程的内存**:根据 ELF 文件的描述加载新进程的内存布局.

3. **设置新进程的寄存器状态**:根据 ELF 文件的入口点和其他信息,设置新进程的寄存器状态.

4. **跳转到新程序的入口点**:开始执行新程序的代码.

### 五、返回用户态

在新进程的上下文被设置好之后,系统调用返回用户态,CPU 开始执行新程序的代码.此时,进程的内容已经完全变成新程序,旧程序的代码和数据已经被替换.

### 六、小结

`execve` 系统调用是 Linux 进程管理中的重要部分,通过 `execve` 调用,一个进程可以完全替换为另一个进程而保持相同的进程 ID.通过分析 `execve` 系统调用,我们可以看到进程的装载过程涉及多个步骤,包括参数复制、内存分配、可执行文件解析和最终的进程替换.这些步骤在 Linux 内核源码中通过多个函数实现,确保进程的安全和正确执行.

通过这种深入的分析,我们可以更好地理解 Linux 系统的进程管理机制以及 `execve` 系统调用在其中的关键作用.

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
- [深入理解 Linux 虚拟内存管理](https://www.xiaolincoding.com/os/3_memory/linux_mem.html#_5-3-%E5%86%85%E6%A0%B8%E5%A6%82%E4%BD%95%E7%AE%A1%E7%90%86%E8%99%9A%E6%8B%9F%E5%86%85%E5%AD%98%E5%8C%BA%E5%9F%9F)