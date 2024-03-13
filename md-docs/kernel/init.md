
# init

在 linux 启动过程中的三个特殊进程 `idle` `init` 和 `kthreadd`

## idle 进程

[/init/main.c](https://github.com/torvalds/linux/blob/v6.6/init/main.c) 文件中的[start_kernel()](https://github.com/torvalds/linux/blob/ffc253263a1375a65fa6c9f62a893e9767fbebfa/init/main.c#L870C6-L870C18)函数是一切的起点,在这个函数被调用之前都是系统的初始化工作(汇编语言),所以对内核的启动分析一般都从这个函数开始

这个函数在执行的过程中初始化、定义了内核中一些十分重要的内容,其执行过程几乎涉及到了内核的所有模块的初始化, 我们这里主要关注**进程初始化**的部分.

首先第一行就使用 [set_task_stack_end_magic(&init_task)](https://github.com/torvalds/linux/blob/ffc253263a1375a65fa6c9f62a893e9767fbebfa/init/main.c#L875) 去初始化 `init_task`, 这个函数本身不稀奇, 只是单纯的为 init_task 添加一个栈溢出的标记保护位, 值得注意的是 `init_task` 变量本身, 它实际上是在 C 中被[初始化定义](https://github.com/torvalds/linux/blob/ffc253263a1375a65fa6c9f62a893e9767fbebfa/init/init_task.c#L64) 的

> 在早期的一些版本(4.4)中也会使用 [INIT_TASK](https://github.com/torvalds/linux/blob/afd2ff9b7e1b367172f18ba7f693dfb62bdcb2dc/init/init_task.c#L18) 宏来进行[展开](https://github.com/torvalds/linux/blob/afd2ff9b7e1b367172f18ba7f693dfb62bdcb2dc/include/linux/init_task.h#L190), 效果是完全相同的

init_task 这个变量的 `comm` 字段的名字是 ["swapper"](https://github.com/torvalds/linux/blob/ffc253263a1375a65fa6c9f62a893e9767fbebfa/include/linux/init_task.h#L38), 这个字段在 `task_struct` 即进程结构体中的含义是[可执行文件的名字](https://github.com/torvalds/linux/blob/ffc253263a1375a65fa6c9f62a893e9767fbebfa/include/linux/sched.h#L1072-L1078)

```c
#define INIT_TASK_COMM "swapper"

struct task_struct init_task = {
    // ...
	.comm = INIT_TASK_COMM
    //
};
```

![20240312122051](https://raw.githubusercontent.com/learner-lu/picbed/master/20240312122051.png)

或者在 gdb 中调试, 使用 `lx_current` 来获取当前进程, 可以看到当前 idle 进程的 pid 为 0, 且 comm 为 "swapper"

```bash
(gdb) p $lx_current().pid
$1 = 0
(gdb) p $lx_current().comm
$2 = "swapper\000\000\000\000\000\000\000\000"
```

> 结构体字段 `.pid` 并没有直接被赋值, 因为在 .data 段而默认初始化为 0

之后start_kernel()函数继续调用各个系统模块进行各种初始化之类的工作, 比如 `trap_init()`是中断向量的相关设置, `mm_init()`是内存管理的设置, `sched_init()`是调度模块的初始化

在执行了上面的各项工作之后,是start_kernel()函数的最后一行代码调用了另一个非常重要的函数 **rest_init()**, [rest_init](https://github.com/torvalds/linux/blob/ffc253263a1375a65fa6c9f62a893e9767fbebfa/init/main.c#L680)

idle进程:

               该进程是Linux中的第一个进程(线程),PID为0;

               idle进程是init进程和kthreadd进程(内核线程)的父进程;

init进程:

               init进程是Linux中第一个用户空间的进程,PID为1;

               init进程是其他用户空间进程的直接或间接父进程;

kthreadd(内核线程):

               kthreadd线程是内核空间其他内核线程的直接或间接父进程,PID为2;

               kthreadd线程负责内核线程的创建工作;

## 参考

- [gitbooks 0xax](https://0xax.gitbooks.io/linux-insides/content/Initialization/linux-initialization-6.html)
- [systemd的作用](https://www.cnblogs.com/linhaostudy/p/8577504.html)
- [[linux]进程(三)_idle进程](https://blog.csdn.net/u013686805/article/details/19905941)
- [Linux下0号进程的前世(init_task进程)今生(idle进程)----Linux进程的管理与调度(五)【转】](https://cloud.tencent.com/developer/article/1339566)
- [Linux中的特殊进程:idle进程、init进程、kthreadd进程](https://blog.csdn.net/JoggingPig/article/details/110239518)
- [<Linux内核分析>(三)_跟踪分析Linux内核的启动过程](https://blog.csdn.net/FIELDOFFIER/article/details/44518597)