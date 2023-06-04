
# init

## 编译过程

把内核从块设备引导加载到内存, 对系统配置参数进行探测, 完成了进入32位保护模式运行之前的所有工作, 位内核系统执行进一步的初始化工作做准备

- bootsect.s => 磁盘引导程序
- setup.s => 获取BIOS参数
- head.s => 运行启动代码程序

Makefile中核心编译环节

```Makefile
all: Image

# 依赖四个文件 => boot/bootsect boot/setup tools/system tools/build
# 是用build 以 $(ROOT_DEV) 为根文件系统设备将其他部分组装成内核映像文件 Image
Image: boot/bootsect boot/setup tools/system tools/build
	tools/build boot/bootsect boot/setup tools/system $(ROOT_DEV) > Image
	sync
# sync : 同步命令, 迫使缓冲块数据立即写盘并更新超级快
```

![20230317180110](https://raw.githubusercontent.com/learner-lu/picbed/master/20230317180110.png)

其中Image依赖的四个文件`boot/bootsect` `boot/setup` `tools/system` `tools/build`分别对应的Makefile规则如下

```Makefile
boot/bootsect:	boot/bootsect.s
	$(AS86) -o boot/bootsect.o $^
	$(LD86) -s -o $@ boot/bootsect.o

boot/setup: boot/setup.s
	$(AS86) -o boot/setup.o $^
	$(LD86) -s -o $@ boot/setup.o

tools/system: boot/head.o init/main.o $(ARCHIVES) $(DRIVERS) $(MATH) $(LIBS)
	$(LD) $(LDFLAGS) $^ -o $@ > System.map

tools/build: tools/build.c
	$(CC) $(CFLAGS) -o $@ $^
```

这里的 `System.map` 用于存放内核符号表信息, 当内核运行出错的时候可以通过 System.map 中的符号表解析查到一个地址对应的变量名, 或反过来通过变量名查到地址

## 引导启动程序

> 引导启动这部分和硬件关系很大, 读不懂理解不了很正常, 跳过此部分直接进入C代码也可以

bootsect.s 和 setup.s 采用近似于 Intel 的汇编语言语法, 需要使用 Intel 8086汇编编译器和链接器as86 ld86

head.s 使用GNU汇编程序格式, 运行在保护模式下, 需要使用 GNU 的as进行编译, 这是一种 AT&T的汇编语言程序

笔者这里实在是不想过多讨论汇编相关的内容, 一个原因是确实枯燥乏味, 一个原因是需要很多硬件相关的前置知识, 最主要的原因是这并不是我关心的问题. 操作系统跑起来很重要但是跑的稳更重要, 深究启动细节或许有些意义, 但对我来说意义不大

下面主要来讨论一下实模式和保护模式

![20230321145946](https://raw.githubusercontent.com/learner-lu/picbed/master/20230321145946.png)

- 实模式: 如左图, 寻址一个内存地址使用段+偏移值, 也就是ds:si, 段值保存在段寄存器ds当中, 段内偏移地址保存在第一可以寻址的寄存器当中(si), 这两个都是16位寄存器, 对应的大小是64KB, 所以偏移地址的范围就是[0,64KB), 所以段的最大长度是64KB.
- 保护模式: 如右图, 寻址一个内存地址仍然是使用段+偏移值, 不过与实模式不同的是, ds并不会直接保存段的地址, 而是保存一个指向全局描述符表(gdt, global descriptor table) 的索引

  gdt中保存着所有的段的信息, 包括内存段的基地址, 偏移值, 段最大长度

  gdt的基地址在gdtr, ds保存一个索引值, 可以使用类似 gdt[ds]的方式得到所需段的基地址

  这样的方式有一些好处, 首先是段的长度可变, 而不是固定死的64KB, 这样变大变小都可以灵活的调整

在 boot/setup.s 中相关的汇编如下

```x86asm
end_move:
	mov	ax,#SETUPSEG	! right, forgot this at first. didn't work :-)
	mov	ds,ax           ! ds 指向本程序的setup段
	lidt	idt_48		! load idt with 0,0
	lgdt	gdt_48		! load gdt with whatever appropriate

idt_48:
	.word	0			! idt limit=0
	.word	0,0			! idt base=0L

gdt_48:
	.word	0x800		! gdt limit=2048, 256 GDT entries
	.word	512+gdt,0x9	! gdt base = 0X9xxxx
```

引导加载程序 bootsect.s 将 setup.s 代码和 system 模块加载到了内存中, 兵器而分别把自己和 setup.s 代码移动到了物理内存,0x90000 和 0x90200 处后, 就把执行权交给了 setup 程序, 其中 system 模块的首部包含有 head.s 代码

setup 程序的主要作用是利用 ROM BIOS 的中断程序去获取机器的一些基本参数, 并保存在 0x90000 开始的内存块中, 以供后面程序使用.. 同时把 system 模块向下移动到物理地址 0x00000 开始处, 这样 system 中的head.s 代码就被移动到 0x00000  处了

然后加载描述符表基地址到描述符表寄存器中, 为进行32为保护模式下的运行做好准备, 接下来对终端控制引荐进行重新设备, 最后通过设计机器控制寄存器 CR0 并跳转到 system 模块的 head.s 代码开始处, 使 CPU 进入32位保护模式下运行

head.s 代码主要作用是初步初始化中断描述符表中256项门描述符, 检查 A20 地址线是否打开, 测试系统是否含有数学协处理器. 然后初始化内存也目录表, 为内存的分页管理做好准备工作

最后跳转到system模块中的初始化程序 init.c 中继续执行

## init 

### 系统调用的过程

下面这四句用了宏 _syscall0 _syscall1 去展开, 相当于 int fork() {...} 这种定义方式

```c
static inline _syscall0(int, fork);
static inline _syscall0(int, pause);
static inline _syscall1(int, setup, void*, BIOS);
static inline _syscall0(int, sync);
```

其中宏展开可以看到内嵌的汇编代码分别如下

```c
#define _syscall0(type, name)                                           \
    type name(void) {                                                   \
        long __res;                                                     \
        __asm__ volatile("int $0x80" : "=a"(__res) : "0"(__NR_##name)); \
        if (__res >= 0) return (type)__res;                             \
        errno = -__res;                                                 \
        return -1;                                                      \
    }

#define _syscall1(type, name, atype, a)                       \
    type name(atype a) {                                      \
        long __res;                                           \
        __asm__ volatile("int $0x80"                          \
                         : "=a"(__res)                        \
                         : "0"(__NR_##name), "b"((long)(a))); \
        if (__res >= 0) return (type)__res;                   \
        errno = -__res;                                       \
        return -1;                                            \
    }
```

在 `include/sys/unistd.h` 中我们可看到如下定义的宏

![20230322135932](https://raw.githubusercontent.com/learner-lu/picbed/master/20230322135932.png)

所以根据对应的映射宏`__NR_##name`展开, 即扩展得到

```c
int fork(void) {
    long __res;
    __asm__ volatile("int $0x80" : "=a"(__res) : "0"(2));
    if (__res >= 0) return (int)__res;
    errno = -__res;
    return -1;
}

int setup(void* BIOS) {
    long __res;
    __asm__ volatile("int $0x80" : "=a"(__res) : "0"(0), "b"((long)(BIOS)));
    if (__res >= 0) return (int)__res;
    errno = -__res;
    return -1;
}
```

这两个函数是通过在 Linux 内核中使用中断 (interrupt) 0x80 来调用系统调用 (system call) 的方式实现的。系统调用是操作系统提供给应用程序的一组接口，应用程序可以通过这些接口来请求操作系统执行特定的操作。

在这两个函数中，`asm volatile("int $0x80")` 的作用是使用内联汇编 (inline assembly) 在代码中嵌入一个汇编指令，将中断 0x80 发送给处理器。这个中断号是 Linux 内核中指向系统调用处理程序的入口点。

__res 变量是用来保存返回值的，通过 "=a"(__res) 表示将 EAX 寄存器中的值赋给 __res 变量。

第一个函数中，"0"(2) 表示将函数调用号 2 (在 Linux 中是 fork() 的函数调用号) 赋给 EAX 寄存器。

第二个函数中，"0"(0) 表示将函数调用号 0 (在 Linux 中是 setup() 的函数调用号) 赋给 EAX 寄存器，"b"((long)(BIOS)) 表示将 BIOS 参数的地址放入 EBX 寄存器中。

接下来，如果返回值大于等于 0，说明系统调用执行成功，将 __res 强制转换为 int 类型并返回。如果返回值小于 0，将 -__res 赋给 errno 变量，并返回 -1，表示系统调用执行失败。 

这里的 fork 对应的索引值为 2, setup 对应的索引值为 0, 与此同时我们可以看到 `include/linux/sys.h` 下的所有系统调用sys_call_table

![20230322140833](https://raw.githubusercontent.com/learner-lu/picbed/master/20230322140833.png)

所以简而言之, 当我们使用fork函数的时候

- fork函数借助其内联汇编: `__asm__ volatile("int $0x80" : "=a"(__res) : "0"(2));`, 中断 0x80 发送给处理器
- CPU接收到中断信号, 转去调用idt的第2号中断, 也就是 `_sys_fork`
- 在 init/main.c 的 main 函数中的 sche_init 初始化的时候设置了相关的中断向量表, 查找到2号中断

  ```c
  set_tss_desc(gdt+FIRST_TSS_ENTRY,&(init_task.task.tss));
  set_ldt_desc(gdt+FIRST_LDT_ENTRY,&(init_task.task.ldt));
  ```

- `_sys_fork` 的实现在 kernel/system_call.s 的汇编代码中

  ```x86asm
  _sys_fork:
  	call _find_empty_process
  	testl %eax,%eax
  	js 1f
  	push %gs
  	pushl %esi
  	pushl %edi
  	pushl %ebp
  	pushl %eax
  	call _copy_process
  	addl $20,%esp
  1:	ret
  ```

- 其中汇编中的两个函数 `_find_empty_process` `_copy_process` 的去掉前缀下划线的函数实现可以在 `kernel/fork.c` 中找到

  ```c
  int copy_process(int nr, long ebp, long edi, long esi, long gs, long none,
                 long ebx, long ecx, long edx, long fs, long es, long ds,
                 long eip, long cs, long eflags, long esp, long ss) {
                    ...
                 }
  int find_empty_process(void) {
    int i;

  repeat:
      if ((++last_pid) < 0)
          last_pid = 1;
      for (i = 0; i < NR_TASKS; i++)
          if (task[i] && task[i]->pid == last_pid)
              goto repeat;
      for (i = 1; i < NR_TASKS; i++)
          if (!task[i])
              return i;
      return -EAGAIN;
  }
  ```

- 全部完成调用之后返回结果: `if (__res >= 0) return (type)__res;`, 或返回错误信息

### main函数部分

main函数的核心部分如下所示, 首先做了一些地址的初始化, 然后初始化各方面的硬件设备. 然后fork一个子进程执行init, 结束之后使用pause等待进程调度

```c
void main(void) {
    // 一些内存地址的初始化
    mem_init(main_memory_start, memory_end);  // 内存初始化     -> mm/memory.c
    trap_init();     // 中断向量初始化 -> kernel/traps.c
    blk_dev_init();  // 块设备初始化   -> kernel/blk_drv/ll_rw_blk.c
    chr_dev_init();  // 字符设备初始化 -> kernel/chr_drv/tty_io.c
    tty_init();      // tty初始化    -> kernel/chr_drv/tty_io.c
    time_init();     // 开机时间初始化
    sched_init();    // 调度程序初始化 -> kernel/sched.c
    buffer_init(buffer_memory_end);  // 缓冲管理初始化 -> fs/buffer.c
    hd_init();            // 硬盘初始化     -> kernel/blk_drv/hd.c
    floppy_init();        // 软驱初始化     -> kernel/blk_drv/floppy.c
    sti();                // 所有初始化工作结束之后开启中断
    move_to_user_mode();  // 进入用户模式

    if (!fork()) {
        init();
    }
    for (;;) pause();
}
```

各部分初始化之后 , 系统已经处理可以运行的状态了, 这时候操作系统进入用户态 `move_to_user_mode`, 将自己移至任务0(进程0)中, 并且使用 fork 创建进程1 (init进程)

---

这里要注意到是, 由于fork创建新进程的过程是通过完全复制父进程代码段和数据段的方式实现的, 因此在首次使用 fork创建新进程并执行 init 时, 为了确保新进程用户态堆栈没有进程 0 的多余信息, 要求进程 0 在创建首个新进程之前不要使用用户态堆栈, **即要求任务0不要调用函数**

因此在 main.c 主函数移动到任务0执行之后, 任务0 中的代码fork不能以函数的形式进行调用, 所以使用了 gcc 函数内嵌汇编的方式

任务0 的pause 也是内联汇编, 因为无法确定父进程和子进程的调用顺序, 所以需要保证这里也不会使用用户态堆栈

---

init中的任务也并不复杂

- 使用 `setup((void*)&drive_info)` 安装根文件系统设备
- 打开设备tty0, 创建终端标准 IO stdin stdout stderr
  - 再次fork在子进程中将终端输入定向到 rc
- 再次fork一个子进程,新建会话开启 /bin/sh. 如果子进程退出则父进程进入死循环继续生成子进程, 等待

```c
void init(void) {
    int pid, i;
    // setup 是一个系统调用
    // 读取硬盘参数包括分区表信息, 加载虚拟盘, 安装根文件系统设备
    setup((void*)&drive_info);
    // 以读写访问方式打开 tty0
    (void)open("/dev/tty0", O_RDWR, 0);  // 0 -> stdin
    (void)dup(0);                        // 复制句柄1号 -> stdout
    (void)dup(0);                        // 复制句柄2号 -> stderr
    // 缓冲区块数
    printf("%d buffers = %d bytes buffer space\n\r", NR_BUFFERS,
           NR_BUFFERS * BLOCK_SIZE);
    // 内存总字节数
    printf("Free mem: %d bytes\n\r", memory_end - main_memory_start);
    // fork 的子进程返回 0
    //        父进程返回子进程 pid
    // 这里只执行子进程
    if (!(pid = fork())) {
        close(0);                          // 关闭 stdin
        if (open("/etc/rc", O_RDONLY, 0))  // 只读打开 /etc/rc
            _exit(1);
        execve("/bin/sh", argv_rc, envp_rc);  // 将进程自身替换成 /bin/sh 程序,
                                              // 参数使用的是 argv_rc envp_rc
        _exit(2);  // 1: 操作未许可 2: 文件/目录不存在
    }
    // 父进程等待子进程结束
    if (pid > 0)
        while (pid != wait(&i)) /* nothing */
            ;
    while (1) {
        // 创建子进程失败
        if ((pid = fork()) < 0) {
            printf("Fork failed in init\r\n");
            continue;
        }
        // 子进程执行
        if (!pid) {
            // 关闭子进程的 stdin stdout stderr
            close(0);
            close(1);
            close(2);
            // 新建会话并设置进程组号
            setsid();
            (void)open("/dev/tty0", O_RDWR, 0);  // 重新打开 tty0 作为 stdin
            (void)dup(0);                        // 复制得到 stdout stderr
            (void)dup(0);
            _exit(execve("/bin/sh", argv,
                         envp));  // 再次执行系统解释程序 /bin/sh,
                                  // 这里的参数选择了 argv envp
        }
        while (1)
            // 父进程等待子进程结束
            if (pid == wait(&i))
                break;
        printf("\n\rchild %d died with code %04x\n\r", pid, i);
        sync();  // 同步操作 刷新缓冲区
    }
    _exit(0); /* NOTE! _exit, not exit() */
    // _exit 是一个 sys_exit 的系统调用
    // exit 是普通函数库中的一个函数
}
```

![20230322153449](https://raw.githubusercontent.com/learner-lu/picbed/master/20230322153449.png)

### 登录

在0.11版本的代码中init函数直接开始执行了/bin/sh, 在现代实际可用的系统中还需要处理多人同时使用系统, 区分用户等能力. 通常程序会根据系统 /etc 目录中的配置文件的设置信息, 对系统中支持的每个终端设备创建子进程, 并在子进程中运行终端初始化设置程序 getty 以提示用于输入登录提示信息 login, 当用户键入用户名之后 getty 替换去执行 login 程序, 验证用户输入口令的正确性之后调用 shell 程序并且进入 shell 交互工作界面

![20230323171144](https://raw.githubusercontent.com/learner-lu/picbed/master/20230323171144.png)

### 进程组 会话(session)

当我们使用如下命令的时候, 此命令包含了三个命令行程序 cat grep more, 并使用管道符 `|` 连接

```bash
cat init/main.c | grep for | more
```

```bash
(base) kamilu@LZX:~/klinux/init$ cat main.c | grep for | more
// 下面这四句用了宏 syscall0 syscall1 去展开, 相当于 int fork() {...}
static inline _syscall0(int, fork);
    // fork 一个进程并在子进程中执行
    if (!fork()) {
    for (;;) pause();
    // fork 的子进程返回 0
    if (!(pid = fork())) {
        if ((pid = fork()) < 0) {
```

这里的三个进程属于同一个进程组

![20230323170440](https://raw.githubusercontent.com/learner-lu/picbed/master/20230323170440.png)

进程组是一个或者多个进程的集合, 每一个进程组都有一个唯一的进程组标识号gid, 每一个进程组有一个称为组长的进程, 组长进程的pid等于进程组的gid

一个进程可以通过setpgid来参加一个现有的进程组或者创建一个新的进程组. 进程组的概念有很多用途, 最常见的是我们在终端上向前台执行程序发出终止信号, 同时终止整个进程组中的所有进程

会话(session) 是一个或多个进程组的集合, 通常情况下用户登录后所执行的所有程序都属于一个会话期, 登陆的shell时会话期的首进程(session header), 当用户退出登录之后 logout, 所有属于我们的会话期的进程都将要被终止, 这是会话期的主要用途之一
