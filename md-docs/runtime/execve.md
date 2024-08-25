
# execve

在一个 shell 中最常见的操作就是执行程序, 此时 shell 会 fork 出一个子进程然后通过调用 `execve()` 来执行对应的任务. 该函数如果成功执行并不会返回, 原先子进程的栈.数据以及堆段会直接被新程序所替换. 当程序结束退出时由父进程 shell 来负责资源的回收(wait)

一个进程一旦调用 exec函数,它本身就"死亡"了,系统把代码段替换成新程序的代码,放弃原有的数据段和堆栈段,并为新程序分配新的数据段与堆栈段,惟一保留的就是进程的 ID.也就是说,对系统而言,还是同一个进程,不过执行的已经是另外一个程序了.

exec() 是一个函数族, 例如 execl/execlp/execle 等等, 略有区别不过最核心的功能都是运行可执行文件

```c
#include <unistd.h>
int execve(const char *filename, char *const argv[], char *const envp[]);
```

前文我们提到了 Linux 下标准的可执行文件格式是 [ELF](./ELF文件格式.md), 但严格来说其实只要一个文件拥有**可执行权限**, 并且可以被内核中的某一种解释器正确加载, 那么他就可以运行.

> 例如以 `#!/bin/bash` 开头的 `.sh` 文件, 或者以 `#!/bin/python3` 开头的 `.py` 文件

linux支持其他不同格式的可执行程序, 在这种方式下, linux能运行其他操作系统所编译的程序, 如MS-DOS程序, 活BSD Unix的COFF可执行格式, 因此linux内核用 `struct linux_binfmt` 来描述各种可执行程序.

```c{4}
struct linux_binfmt {
	struct list_head lh;
	struct module *module;
	int (*load_binary)(struct linux_binprm *);
	int (*load_shlib)(struct file *);
#ifdef CONFIG_COREDUMP
	int (*core_dump)(struct coredump_params *cprm);
	unsigned long min_coredump;	/* minimal dump size */
#endif
} __randomize_layout;
```

其提供了3种方法来加载和执行可执行程序

- load_binary: 通过读存放在可执行文件中的信息为当前进程建立一个新的执行环境
- load_shlib: 用于动态的把一个共享库捆绑到一个已经在运行的进程, 这是由uselib()系统调用激活的
- core_dump: 在名为core的文件中, 存放当前进程的执行上下文. 这个文件通常是在进程接收到一个缺省操作为"dump"的信号时被创建的, 其格式取决于被执行程序的可执行类型

当我们执行一个可执行程序的时候, 内核会 `list_for_each_entry` 遍历所有注册的linux_binfmt对象, 对其调用 `load_binrary` 方法来尝试加载, 直到加载成功为止.

linux内核对所支持的每种可执行的程序类型都有个struct linux_binfmt的数据结构,定义如下

所有支持的格式都需要通过 `register_binfmt` 函数注册到内核当中, 该函数的作用就是将一个结构体头插到 `format` 全局变量的链表中

```c{10,11}
/* Registration of default binfmt handlers */
static inline void register_binfmt(struct linux_binfmt *fmt)
{
	__register_binfmt(fmt, 0);
}

void __register_binfmt(struct linux_binfmt * fmt, int insert)
{
	write_lock(&binfmt_lock);
	insert ? list_add(&fmt->lh, &formats) :
		 list_add_tail(&fmt->lh, &formats);
	write_unlock(&binfmt_lock);
}
```

在代码库中全局搜索该函数, 可以看到内核支持的所有可执行文件格式

```bash
grep -rnw . -e 'register_binfmt' --include \*.c --include \*.h
```

![20240621154352](https://raw.githubusercontent.com/learner-lu/picbed/master/20240621154352.png)

> 6.6 相比于 4.4.6 删除了对于 em86/a.out 格式的支持
> 
> 对应的注册位置见 [binfmt_elf.c](https://github.com/luzhixing12345/klinux/blob/98b724b922a1949f03d5b53f710a97753d76a338/fs/binfmt_elf.c#L2163-L2167) [binfmt_flat.c](https://github.com/luzhixing12345/klinux/blob/98b724b922a1949f03d5b53f710a97753d76a338/fs/binfmt_flat.c#L937-L941) [binfmt_script.c](https://github.com/luzhixing12345/klinux/blob/98b724b922a1949f03d5b53f710a97753d76a338/fs/binfmt_script.c#L145-L149)


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
- [ld-linux.so 加载流程](https://zhuanlan.zhihu.com/p/690824245)
- [Linux进程启动过程分析do_execve(可执行程序的加载和运行)---Linux进程的管理与调度(十一)](https://www.cnblogs.com/linhaostudy/p/9650228.html)