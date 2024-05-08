
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

## 多线程安全问题

在Linux内核的早期版本中,更新`struct file`中的偏移量存在多线程安全问题.这是因为在多线程环境下,如果没有适当的同步机制,可能会导致多个线程同时修改同一个文件偏移量,从而引发数据竞争和不一致的问题.

这个问题直到Linux 3.14版本才得到修复,该版本在2014年3月底发布.因此,在Linux 3.14之前的版本,如Ubuntu 14.04,默认情况下的`write(2)`系统调用并不保证线程安全性.

## 参考

- [Linux 内核文件描述符表的演变](https://zhuanlan.zhihu.com/p/34280875)
- [File descriptor](https://en.wikipedia.org/wiki/File_descriptor)