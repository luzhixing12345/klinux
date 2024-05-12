
# fd

在Linux内核中,`struct task_struct`、`struct file`和`struct inode`是三个核心的数据结构,它们共同维护着进程、打开的文件以及文件本身的信息
.
- `struct task_struct`: 这个结构体用于表示系统中的每一个进程和线程.它包含了进程的多种信息,例如进程的状态、调度信息、内存信息、以及文件描述符等.

  > 在 [proc/task_struct](../proc/task_struct.md) 一节中比较详细的介绍了相关的结构体字段

- `struct file`: 当一个文件在Linux系统中被打开时,会创建一个`struct file`对象.这个对象包含了特定于打开文件的信息,如文件的偏移量(offset).不同的进程或线程打开同一个文件时,会有不同的`struct file`对象,因为**每个打开的文件都有自己的偏移量和文件状态**.
- `struct inode`: 表示文件系统中的一个文件或目录.它存储了文件的**元数据**,如文件大小、权限、时间戳(创建时间、修改时间等).一个`struct inode`对象对应于文件系统中的一个唯一文件或目录,**即使该文件被多次打开**,所有相关的`struct file`对象都会**指向同一个**`struct inode`对象.

简而言之, `struct file` 和 `struct inode` 的区别和联系主要在于:

- **文件偏移量**:每个`struct file`对象都有自己的偏移量,表示该文件在进程中的读取/写入位置.例如,一个web服务器可能从文件末尾追加日志,而用户使用`less`查看同一个日志文件时,可能从文件开头开始读取.
- **文件元数据**:文件的元数据,如长度、权限等,存储在`struct inode`中.这意味着,即使多个进程打开了同一个文件,它们看到的文件属性(如通过`ls -l`或`stat(2)`系统调用获取的)都是一致的.

## fd 的演进

### 版本 A:从 0.01 到 1.1.10

最早的 Linux 内核直接把元素为 `struct file*` 的定长数组放在 `struct task_struct` 里

```c
// include/linux/sched.h of linux-1.1.10

struct task_struct {
        // ...
        struct file * filp[NR_OPEN];
        fd_set close_on_exec;
        // ...
};
```

从 int fd 取到 struct file* fp 的写法是:

```c
struct file* fp = current->filp[fd];
```

而 `struct file` 和 `struct inode` 也是位于各自的定长数组中

```c
// fs/file_table.c of linux-0.99

struct file file_table[NR_FILE];

// fs/inode.c of linux-0.99

static struct inode inode_table[NR_INODE];
```

NR_OPEN、NR_FILE、NR_INODE 这几个宏的值决定了上述数组的大小,它们的值逐渐增大.修改 NR_OPEN 会影响 sizeof (struct task_struct),也会直接影响每个进程占用的物理内存的大小,因为 **task_struct 对象是不会 swap to disk 的**

在 0.99.10 中,`struct file` 和 `struct inode` 改成了动态分配,这样整个系统能同时打开的文件数大大增加,但每个进程能打开的文件数还是 NR_OPEN

```diff
// fs/file_table.c of linux-0.99.10

-struct file file_table[NR_FILE];
+struct file * first_file;
```

### 版本 B:1.1.11 到 1.3.21

```c
// include/linux/sched.h of linux-1.3.21

/* Open file table structure */
struct files_struct {
        int count;
        fd_set close_on_exec;
        struct file * fd[NR_OPEN];
};

struct task_struct {
        // ...

/* filesystem information */
        struct fs_struct fs[1];
/* open file information */
        struct files_struct files[1];
/* memory management info */
        struct mm_struct mm[1];

        // ...
};
```

这样做没有改变程序的功能,只是更好地组织了数据结构,让紧密相关的数据成员位于同一个结构体中,体现了封装的思想.修改 NR_OPEN 也会直接影响 sizeof (struct task_struct).

> 这里为什么要用长度为 1 的 struct 数组,而不直接放 struct,我猜是为了将来改成指针时不必修改客户代码

从 int fd 取到 struct file* fp 的写法变成:

```c
struct file* fp = current->files->fd[fd];
```

### 版本 C:1.3.22 到 2.1.89

1.3.22 把 task_struct 的 files、fs、mm 等成员变成了指针,让 sizeof(struct task_struct) 瘦身了很多.这么做是为了支持多线程.

```diff
// include/linux/sched.h of linux-2.0.2

struct task_struct {
        // ...

 /* filesystem information */
-       struct fs_struct fs[1];
+       struct fs_struct *fs;
 /* open file information */
-       struct files_struct files[1];
+       struct files_struct *files;
 /* memory management info */
-       struct mm_struct mm[1];
+       struct mm_struct *mm;

        // ...
};
```

从 int fd 取到 struct file* fp 的写法不变,还是 current->files->fd[fd].

Linux 2.0 开始支持多线程.(最早是 LinuxThreads 实现,2.6 改成了更符合 POSIX 语义的 NPTL 实现.)把 files_struct 成员从 task_struct 里移出来,让同一进程内的多个线程可以共享一个 files_struct 对象,这样线程 1 打开的文件自然就能被线程 2 看到了.

![20240508200928](https://raw.githubusercontent.com/learner-lu/picbed/master/20240508200928.png)

此时同一进程内的两个线程共享 files_struct 对象, fs_struct 和 mm_struct 也是同理

### 版本 D:2.1.90 到 2.6.13

2.1.90 把 files_struct 的 fd 成员从定长数组改成了动态数组,这样每个进程就能同时打开很多文件了,为编写高并发的网络服务扫清了一大障碍

```diff
// include/linux/sched.h of linux-2.2.0

/*
 * Open file table structure
 */
struct files_struct {
        atomic_t count;
+       int max_fds;
+       struct file ** fd;      /* current fd array */
        fd_set close_on_exec;   // changed to fd_set* in 2.2.12
        fd_set open_fds;
-       struct file * fd[NR_OPEN];
};
```

此时数据结构示意图如下

![20240508201043](https://raw.githubusercontent.com/learner-lu/picbed/master/20240508201043.png)

从 int fd 取到 struct file* fp 的写法不变,还是 current->files->fd[fd].

至此,文件描述符表的功能已经完善,下一个版本是性能的改进

### 版本 E:2.6.14 至今

2.6.14 引入了 struct fdtable 作为 files_struct 的间接成员,把 fd、max_fds、close_on_exec 等成员[移入 fdtable](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=badf16621c1f9d1ac753be056fce11b43d6e0be5).这么做是为了方便采用 RCU,让 fdtable 可以整体替换.Read-Copy Update ([RCU](../kernel/rcu.md)) 是 Paul E. McKenney 的杰作,是内核广泛采用的一种伸缩性更好的读写同步机制

```c
// include/linux/fdtable.h of linux-2.6.37

struct fdtable {
        unsigned int max_fds;
        struct file __rcu **fd;      /* current fd array */
        fd_set *close_on_exec;
        fd_set *open_fds;
        struct rcu_head rcu;
        struct fdtable *next;
};

/*
 * Open file table structure
 */
struct files_struct {
  /*
   * read mostly part
   */
        atomic_t count;
        struct fdtable __rcu *fdt;
        struct fdtable fdtab;
  /*
   * written part on a separate cache line in SMP
   */
        spinlock_t file_lock ____cacheline_aligned_in_smp;
        int next_fd;
        struct embedded_fd_set close_on_exec_init;
        struct embedded_fd_set open_fds_init;
        struct file __rcu * fd_array[NR_OPEN_DEFAULT];
};
```

数据结构示意图如下:

![20240508201310](https://raw.githubusercontent.com/learner-lu/picbed/master/20240508201310.png)

从 int fd 取到 struct file* fp 的途径变成, 实际的代码比这个要复杂,因为 files->fdt 这一步要用 rcu_dereference 来做(上图的红线)

```c
current->files->fdt->fd[fd];
```

fdtable 中的 max_fds 记录一个进程最大可以打开的文件的个数, 可以使用如下指令查看

```bash
cat /proc/sys/fs/file-max
```

## 文件的打开

当一个进程打开文件的时候, 操作系统内核的数据成员链如下所示

![20240511172159](https://raw.githubusercontent.com/learner-lu/picbed/master/20240511172159.png)

> 这里的 inode 用于保存所有的文件字节数据和元数据, 更多内容见 [inode](./inode.md)

首先每一个进程维护自己内部的文件描述符(fd), **不同进程看到的 fd 是不同的**. 每当打开一个文件的时候会分配一个**最小可用的fd**. 当 close 文件之后该 fd 会被回收

> 默认 fd 0/1/2 分别对应 stdin, stdout, stderr

`file` 结构体字段中

- `f_inode` 指向文件对应的 inode, 也就是文件实际对应的字节数据和元数据.
- `f_count` 用于跟踪当前有多少个引用指向这个 file 结构体.每当一个进程打开一个文件并获得一个 file 结构体的引用时,这个计数会增加.当进程关闭文件或者释放对 file 结构体的引用时,计数会减少

  当 f_count 不为零时,表示还有引用存在,因此内核不会释放 file 结构体相关的资源.只有当 f_count 减少到零时,才表示没有进程再使用这个 file 结构体,内核可以安全地释放与其相关的资源

  > `atomic_long_t` 类型保证了对 f_count 的增减操作是原子的
  >
  > 上图中 files_struct 中的 count 也是相同的作用

- `f_flags` 用于表示打开文件时的各种状态标志和文件描述符的标志, 使用 `open("xxx", flags)` 打开的文件的标志(例如 `O_APPEND` `O_ASYNC`) 都保存在此处

  > 比较特殊的是 `O_CLOEXEC`, 它会被单独保存在 `fdtable` 中的 `close_on_exec` 中, 而非 flag 中

- `f_pos` 偏移量, 每个打开的文件的偏移量保存在 file 中
- `fp` 与文件系统操作相关的字段, 所有对于该文件的 open/write/access 等操作都会通过 vfs 层传递到文件系统执行

如果**多个进程打开了同一个文件**, 此时它们会创建自己**独立**的 `file`, 但是指向**相同**的 `inode`

![20240511172717](https://raw.githubusercontent.com/learner-lu/picbed/master/20240511172717.png)

但如果是同一进程中的不同线程打开了同一个文件, 那么它们共享 fdt

![20240511173303](https://raw.githubusercontent.com/learner-lu/picbed/master/20240511173303.png)

```c
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

// 线程函数,接收一个整数参数作为线程号
void* thread_function(void* arg) {
    int thread_id = *((int *)arg); // 强制转换void*参数为int*
    const char* filename = "a.c"; // 所有线程打开相同的文件

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return NULL;
    }

    // 打印线程号和文件描述符
    printf("Thread ID %d - File '%s' opened with FD: %d\n", thread_id, filename, fd);
    sleep(1); // 加一点延迟, 不然可能一个线程快速执行结束释放 fd
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
```

编译运行可以发现文件描述符 fd 是顺次增长的, 而并非独立的, 由此可以验证结论

```bash
(base) kamilu@LZX:~/libc$ gcc a.c -lpthread -o a
(base) kamilu@LZX:~/libc$ ./a
Thread ID 1 - File 'a.c' opened with FD: 3
Thread ID 2 - File 'a.c' opened with FD: 4
Thread ID 1 - File 'a.c' closed with FD: 3
Thread ID 2 - File 'a.c' closed with FD: 4
```

> 在Linux内核的早期版本中,更新`struct file`中的偏移量存在多线程安全问题.这是因为在多线程环境下,如果没有适当的同步机制,可能会导致多个线程同时修改同一个文件偏移量,从而引发数据竞争和不一致的问题.
>
> 这个问题直到Linux 3.14版本才得到修复,该版本在2014年3月底发布.因此,在Linux 3.14之前的版本,如Ubuntu 14.04,默认情况下的`write(2)`系统调用并不保证线程安全性.

下面我们考虑一些比较复杂的情况

- **open+open**: 文件连续打开两次, 每一次 open 会创建一个 fd 中创建一个表项, 对应两个不同的 `file`. 如果分别使用 write 对 fd 进行写入则由于打开文件的偏移量默认为 0, 后写入的内容会覆盖前面的内容

  ![20240511182428](https://raw.githubusercontent.com/learner-lu/picbed/master/20240511182428.png)

  > 测试代码见 [open_open.c](https://github.com/luzhixing12345/libc/blob/main/examples/fs/open_open.c)

- **open+dup**: 打开一个文件然后dup得到一个新的文件描述符, 此时虽然有两个 fd 但是对应同一个 `file`, 共享包括 flag/pos 的信息. 此时 `file` 中的 `f_count` 引用计数为 2

  ![20240511183938](https://raw.githubusercontent.com/learner-lu/picbed/master/20240511183938.png)

  > 测试代码见 [open_dup.c](https://github.com/luzhixing12345/libc/blob/main/examples/fs/open_dup.c)

- **open+fork**: 打开一个文件然后 fork, 此时会有两个 task_struct, 但是它们共享同一个 `file`, 共享相同的偏移量, 因此父子进程可以分别 write 而不会覆盖

  ![20240511190013](https://raw.githubusercontent.com/learner-lu/picbed/master/20240511190013.png)

  > 测试代码见 [open_fork.c](https://github.com/luzhixing12345/libc/blob/main/examples/fs/open_fork.c)

## 参考

- [Linux 内核文件描述符表的演变](https://zhuanlan.zhihu.com/p/34280875)
- [File descriptor](https://en.wikipedia.org/wiki/File_descriptor)
- [Linux 文件系统(三):分块读写](https://www.bilibili.com/video/BV1QT411r738/)