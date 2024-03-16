
# init

在 linux 启动过程中的三个特殊进程 `idle` `init` 和 `kthreadd`

## idle 进程

[/init/main.c](https://github.com/torvalds/linux/blob/v6.6/init/main.c) 文件中的[start_kernel()](https://github.com/torvalds/linux/blob/ffc253263a1375a65fa6c9f62a893e9767fbebfa/init/main.c#L870C6-L870C18)函数是一切的起点,在这个函数被调用之前都是系统的初始化工作(汇编语言),所以对内核的启动分析一般都从这个函数开始

这个函数在执行的过程中初始化、定义了内核中一些十分重要的内容,其执行过程几乎涉及到了内核的所有模块的初始化, 我们这里主要关注**进程初始化**的部分.

```c
void __init start_kernel(void)
{
	char *command_line;
	char *after_dashes;

	set_task_stack_end_magic(&init_task);
    // 一大堆 init
    rest_init();
}
```

首先第一行就使用 [set_task_stack_end_magic(&init_task)](https://github.com/torvalds/linux/blob/ffc253263a1375a65fa6c9f62a893e9767fbebfa/init/main.c#L875) 去初始化 `init_task`, 这个函数本身不稀奇, 只是单纯的为 init_task 添加一个栈溢出的标记保护位, 值得注意的是 `init_task` 变量本身, 它实际上是在 C 中被[初始化定义](https://github.com/torvalds/linux/blob/ffc253263a1375a65fa6c9f62a893e9767fbebfa/init/init_task.c#L64) 的

> 在早期的一些版本(4.4)中也会使用 [INIT_TASK](https://github.com/torvalds/linux/blob/afd2ff9b7e1b367172f18ba7f693dfb62bdcb2dc/init/init_task.c#L18) 宏来进行[展开](https://github.com/torvalds/linux/blob/afd2ff9b7e1b367172f18ba7f693dfb62bdcb2dc/include/linux/init_task.h#L190), 效果是完全相同的

init_task 这个变量的 `comm` 字段的名字是 ["swapper"](https://github.com/torvalds/linux/blob/ffc253263a1375a65fa6c9f62a893e9767fbebfa/include/linux/init_task.h#L38), 这个字段在 `task_struct` 即进程结构体中的含义是[可执行文件的名字](https://github.com/torvalds/linux/blob/ffc253263a1375a65fa6c9f62a893e9767fbebfa/include/linux/sched.h#L1072-L1078)

> 初始(idle)进程的名字 swapper 其实就是从 UNIX 时代延续下来的, 曾经也有人给 linux 提过 [patch](https://www.uwsg.indiana.edu/hypermail/linux/kernel/0604.2/1270.html) 想要将名字改为 "idle" 但被拒绝了
>
>> Yes, swapper is because of historical reasons. In most text books for
>> Unix, the initial process on boot up is called "swapper". Probably
>> because those early Unix systems had this process handle the swapping
>> (as kswapd does today).
>> 
>> By doing this, it will probably make Linux out of sync with all the text
>> books on Unix, so it really is Linus' call.
>
> 回复的意思大致是说 swapper 的名字只是为了和 UNIX 系统保持一致

```c
#define INIT_TASK_COMM "swapper"

struct task_struct init_task = {
    // ...
	.comm = INIT_TASK_COMM
    //
    .prio = MAX_PRIO-20, // 优先级最低
    .mm = NULL // 由于init_task是一个运行在内核空间的内核线程, 因此其虚地址段mm为NULL
};
```

我们可以在开头打一个断点查看 init_task.comm 的字段, 确实为 "swapper"

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

idle 运行在**内核态**, 在执行了上面的各项工作之后, 在 start_kernel() 函数的最后一行代码调用了另一个非常重要的函数 [rest_init](https://github.com/torvalds/linux/blob/ffc253263a1375a65fa6c9f62a893e9767fbebfa/init/main.c#L680), 创建了两个进程 `kernel_init` 和 `kthreadd`

```c
void rest_init(void)
{
    // ...
	kernel_thread(kernel_init, NULL, CLONE_FS); // 创建 1 号内核线程
	pid = kernel_thread(kthreadd, NULL, CLONE_FS | CLONE_FILES); // 创建 2 号内核线程
	// ...
}
```

> 6.6 的 kernel 使用的是 [user_mode_thread](https://github.com/torvalds/linux/blob/ffc253263a1375a65fa6c9f62a893e9767fbebfa/init/main.c#L691) 来创建用户级进程 "kernel_init", 它和使用 kernel_thread 创建的没有[区别](https://github.com/torvalds/linux/blob/480e035fc4c714fb5536e64ab9db04fedc89e910/kernel/fork.c#L2845-L2875)

kernel_thread 调用 _do_fork 创建一个新的**线程**, 创建之初是线程, 随后后会**演变为进程**, 但此时需要注意的是该线程**还并没有被调度执行**

```c
/*
 * Create a kernel thread.
 */
pid_t kernel_thread(int (*fn)(void *), void *arg, unsigned long flags)
{
	return _do_fork(flags|CLONE_VM|CLONE_UNTRACED, (unsigned long)fn,
		(unsigned long)arg, NULL, NULL, 0);
}
```

> 6.6 实际调用的是 [kernel_clone](https://github.com/torvalds/linux/blob/480e035fc4c714fb5536e64ab9db04fedc89e910/kernel/fork.c#L2755), 原理与 _do_fork 类似

可以看到创建完第二个内核线程之后 pid 的值为 2

![20240314231716](https://raw.githubusercontent.com/learner-lu/picbed/master/20240314231716.png)

我们暂且不讨论 init 和 kthreadd, 将 idle 进程处理完

init_idle_bootup_task():当前0号进程init_task最终会退化成idle进程,所以这里调用init_idle_bootup_task()函数,让init_task进程隶属到idle调度类中.即选择idle的调度相关函数.

```c
static struct task_struct *get_current(void)
{
	return this_cpu_read_stable(current_task);
}

#define current get_current()
```

这里不会深入探讨有关 kernel_thread 实现的细节(我们将在描述调度程序详细展开).现在我们只需要知道我们用 kernel_thread 函数创建了新的内核线程 ,其中包含 `PID = 1` 的 init 进程和 `PID = 2` 的 kthreadd 进程, 进入 shell 之后也可以使用 ps 查看

![20240314231941](https://raw.githubusercontent.com/learner-lu/picbed/master/20240314231941.png)

idle 进程初始化的时候进程的优先级被设置为了 `.prio = MAX_PRIO-20`(120), 数值越低代表优先级越高, 因此 idle 实际上是优先级最低的一个进程

idle 进程最后调用 cpu_startup_entry, 调用 cpu_idle_loop, 简化后的执行逻辑如下, 当系统无事可做时,会调度其执行, 此时该内核会变为idle进程,让出CPU,自己进入睡眠, 进入一个无限循环

```c
void cpu_idle(void) {
    /* endless idle loop with no priority at all */
    while (1) {
        while (!need_resched()) {
            if (cpu_is_offline(smp_processor_id())) {
                tick_set_cpu_plugoff_flag(1);
                cpu_die(); /* plugoff CPU */
            }
            if (cpuidle_idle_call())
                pm_idle(); /* 进入低电 */
        }
        schedule_preempt_disabled(); /* 调用schedule() */
    }
}
```

> 早先版本中,idle是参与调度的,所以将其优先级设低点,当没有其他进程可以运行时,才会调度执行 idle.而目前的版本中idle并不在运行队列中参与调度,而是在运行队列结构中含idle指针,指向idle进程,在调度器发现运行队列为空的时候运行,调入运行

## init

在rest_init函数中,内核将通过下面的代码产生第一个真正的进程(pid=1):

```c
kernel_thread(kernel_init, NULL, CLONE_FS);
```

这个进程就是着名的pid为1的init进程,它会继续完成剩下的初始化工作,然后execve(/sbin/init), 成为系统中的其他所有进程的祖先

init进程应该是一个用户空间的进程, 但是这里却是**通过kernel_thread的方式创建**的, 哪岂不是一个永远运行在**内核态的内核线程**么, 它是怎么演变为真正意义上**用户空间的init进程**的?

1号kernel_init进程完成linux的各项配置后(kernel_init_freeable),就会尝试执行 init 程序, 如果没有手动指定 init 位置会在 `sbin/init` `etc/init` `bin/init` 等位置搜索, 如果都没有找到则进入 panic.

> 可以在 qemu 启动时的传递参数包含了 rdinit=sbin/init 来指定位置, 此时会进入 ramdisk_execute_command 分支执行

```c
static int kernel_init(void *unused)
{
	int ret;

	kernel_init_freeable();

    // ...

    // qemu 启动时的传递参数包含了 rdinit=sbin/init
    // 该路径位于制作的 initramfs 中的 sbin/init 的位置
	if (ramdisk_execute_command) {
		ret = run_init_process(ramdisk_execute_command); // 执行 init
		if (!ret)
			return 0;
		pr_err("Failed to execute %s (error %d)\n",
		       ramdisk_execute_command, ret);
	}

	/*
	 * We try each of these until one succeeds.
	 *
	 * The Bourne shell can be used instead of init if we are
	 * trying to recover a really broken machine.
	 */
	if (execute_command) {
		ret = run_init_process(execute_command);
		if (!ret)
			return 0;
		panic("Requested init %s failed (error %d).",
		      execute_command, ret);
	}
    // 寻找其他可能的进程执行
	if (!try_to_run_init_process("/sbin/init") ||
	    !try_to_run_init_process("/etc/init") ||
	    !try_to_run_init_process("/bin/init") ||
	    !try_to_run_init_process("/bin/sh"))
		return 0;

	panic("No working init found.  Try passing init= option to kernel. "
	      "See Linux Documentation/init.txt for guidance.");
}
```

run_init_process 就是使用 do_execve 将核心进程kernel_init转换成用户进程init

```c
static int run_init_process(const char *init_filename)
{
	argv_init[0] = init_filename;
	return do_execve(getname_kernel(init_filename),
		(const char __user *const __user *)argv_init,
		(const char __user *const __user *)envp_init);
}
```

## kthreadd

在rest_init函数中,内核将通过下面的代码产生第一个kthreadd(pid=2)

```c
kernel_thread(kthreadd, NULL, CLONE_FS | CLONE_FILES);
```

它的任务就是**管理和调度其他内核线程** kernel_thread, 会循环执行一个kthread的函数,该函数的作用就是运行kthread_create_list全局链表中维护的kthread, 当我们调用kernel_thread创建的内核线程会被加入到此链表中,因此所有的内核线程都是直接或者间接的以kthreadd为父进程

```c
#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

int kthreadd(void *unused) {
	struct task_struct *tsk = current;

	/* Setup a clean context for our children to inherit. */
	set_task_comm(tsk, "kthreadd"); // 设置名字为 kthreadd

	for (;;) {
        // 找到内核线程列表中的一个进行调度执行
		while (!list_empty(&kthread_create_list)) {
			struct kthread_create_info *create;

			create = list_entry(kthread_create_list.next,
					    struct kthread_create_info, list);
			list_del_init(&create->list);
			create_kthread(create);
		}
	}

	return 0;
}
```

## 参考

- [end of initialization](https://0xax.gitbooks.io/linux-insides/content/Initialization/linux-initialization-10.html)
- [systemd的作用](https://www.cnblogs.com/linhaostudy/p/8577504.html)
- [[linux]进程(三)_idle进程](https://blog.csdn.net/u013686805/article/details/19905941)
- [Linux下0号进程的前世(init_task进程)今生(idle进程)----Linux进程的管理与调度(五)【转】](https://cloud.tencent.com/developer/article/1339566)
- [Linux中的特殊进程:idle进程、init进程、kthreadd进程](https://blog.csdn.net/JoggingPig/article/details/110239518)
- [<Linux内核分析>(三)_跟踪分析Linux内核的启动过程](https://blog.csdn.net/FIELDOFFIER/article/details/44518597)