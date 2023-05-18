/*
 *  linux/init/main.c
 *
 *  (C) 1991  Linus Torvalds
 */

// 包括定义在 unistd.h 中的内嵌汇编代码等信息
// -> include/sys/unistd.h/ -> #ifdef __LIBRARY__
#define __LIBRARY__

#include <time.h>    // 时间类型头文件
#include <unistd.h>  // 标准符号常数与类型文件

// 下面这四句用了宏 syscall0 syscall1 去展开, 相当于 int fork() {...}
// 这种定义方式
static inline _syscall0(int, fork);
static inline _syscall0(int, pause);
static inline _syscall1(int, setup, void *,
                        BIOS);  // -> kernel/blk_drv.c -> sys_setup
static inline _syscall0(int, sync);

#include <asm/io.h>  // io 头文件, 以宏的嵌入汇编程序形式定义对io端口操作的函数
#include <asm/system.h>  // 系统头文件, 以宏的形式定义了许多有关设置或者修改
#include <fcntl.h>  // 文件控制头文件, 用于问火箭机器描述如的操作控制常数符号的定义
#include <linux/fs.h>  // 文件系统头文件, 定义文件表结构 (file buffer_head m_inode)
#include <linux/head.h>  // head头文件, 定义了段描述符的简单结构, 和几个选择符常量
#include <linux/sched.h>  // 调度程序头文件
#include <linux/tty.h>    // 终端头文件
#include <stdarg.h>     // 标准参数头文件 va_list va_start va_arg va_end
#include <stddef.h>     // 标准定义头文件 NULL offsetof
#include <sys/types.h>  // 类型头文件

static char printbuf[1024];  // 内核显示信息的缓存

extern int vsprintf();           // 送格式化输出到一个字符串中
extern void init(void);          // 初始化
extern void blk_dev_init(void);  // 块设备初始化子程序
extern void chr_dev_init(void);  // 字符设备初始化
extern void hd_init(void);       // 硬盘初始化程序
extern void floppy_init(void);   // 软驱初始化程序
extern void mem_init(long start, long end);       // 内存管理初始化
extern long rd_init(long mem_start, int length);  // 虚拟盘初始化
extern long kernel_mktime(struct tm *tm);  // 计算系统开机启动时间
extern long startup_time;  // 内核启动时间(开机多少秒)

// setup.s 在引导的时候设置的
#define EXT_MEM_K (*(unsigned short *)0x90002)  // 1M 以后扩展内存的大小KB
#define DRIVE_INFO (*(struct drive_info *)0x90080)  // 硬盘参数表基地址
#define ORIG_ROOT_DEV (*(unsigned short *)0x901FC)  // 根文件系统所在设备号

// 实时时钟
#define CMOS_READ(addr)            \
    ({                             \
        outb_p(0x80 | addr, 0x70); \
        inb_p(0x71);               \
    })

// 将 BCD 码转成二进制
#define BCD_TO_BIN(val) ((val) = ((val)&15) + ((val) >> 4) * 10)

// 取 CMOS 时钟 -> 转换为二进制
// 设置开机时间: startup_time = kernel_mktime(&time)
static void time_init(void) {
    struct tm time;

    do {
        time.tm_sec = CMOS_READ(0);
        time.tm_min = CMOS_READ(2);
        time.tm_hour = CMOS_READ(4);
        time.tm_mday = CMOS_READ(7);
        time.tm_mon = CMOS_READ(8);
        time.tm_year = CMOS_READ(9);
    } while (time.tm_sec != CMOS_READ(0));
    BCD_TO_BIN(time.tm_sec);
    BCD_TO_BIN(time.tm_min);
    BCD_TO_BIN(time.tm_hour);
    BCD_TO_BIN(time.tm_mday);
    BCD_TO_BIN(time.tm_mon);
    BCD_TO_BIN(time.tm_year);
    time.tm_mon--;
    startup_time = kernel_mktime(&time);
}

static long memory_end = 0;  // 机器具有的物理内存容量(字节数)
static long buffer_memory_end = 0;  // 高速缓冲区末端地址
static long main_memory_start = 0;  // 主内存开始位置

// 硬盘参数表
struct drive_info {
    char dummy[32];
} drive_info;

void main(void) {
    // <- 此时中断仍然被禁止着, 完成必要的设置之后就将其开启
    ROOT_DEV = ORIG_ROOT_DEV;  // 根设备号ROOT_DEV(fs/super.c) = #define 0x901FC
    drive_info = DRIVE_INFO;  // 硬盘参数表基地址 0x90080

    // 内存大小 = 1Mb + 扩展内存(k) * 1024
    // 小于 4kb(1页) 的页被忽略
    memory_end = (1 << 20) + (EXT_MEM_K << 10);
    memory_end &= 0xfffff000;

    // 内存 > 16 Mb -> 16 Mb
    if (memory_end > 16 * 1024 * 1024) {
        memory_end = 16 * 1024 * 1024;
    }
    // 内存 > 12/6/ Mb , 缓冲区末端 4/2/1 Mb
    if (memory_end > 12 * 1024 * 1024) {
        buffer_memory_end = 4 * 1024 * 1024;
    } else if (memory_end > 6 * 1024 * 1024) {
        buffer_memory_end = 2 * 1024 * 1024;
    } else {
        buffer_memory_end = 1 * 1024 * 1024;
    }
    // 主存起始位置 = 缓冲区末端
    main_memory_start = buffer_memory_end;
#ifdef RAMDISK
    // 如果定义了内存虚拟盘, 则初始化虚拟盘 -> kernel/blk_drv/ramdisk.c
    main_memory_start += rd_init(main_memory_start, RAMDISK * 1024);
#endif
    mem_init(main_memory_start,
             memory_end);  // 内存初始化     -> mm/memory.c
    trap_init();           // 中断向量初始化 -> kernel/traps.c
    blk_dev_init();        // 块设备初始化   -> kernel/blk_drv/ll_rw_blk.c
    chr_dev_init();        // 字符设备初始化 -> kernel/chr_drv/tty_io.c
    tty_init();            // tty初始化    -> kernel/chr_drv/tty_io.c
    time_init();           // 开机时间初始化
    sched_init();          // 调度程序初始化 -> kernel/sched.c
    buffer_init(buffer_memory_end);  // 缓冲管理初始化 -> fs/buffer.c
    hd_init();            // 硬盘初始化     -> kernel/blk_drv/hd.c
    floppy_init();        // 软驱初始化     -> kernel/blk_drv/floppy.c
    sti();                // 所有初始化工作结束之后开启中断
    move_to_user_mode();  // 进入用户模式

    // fork 一个进程并在子进程中执行
    if (!fork()) {
        init();
    }
    /*
     *   NOTE!!   For any other task 'pause()' would mean we have to get a
     * signal to awaken, but task0 is the sole exception (see 'schedule()')
     * as task 0 gets activated at every idle moment (when no other tasks
     * can run). For task0 'pause()' just means we go check if some other
     * task can run, and if not we return here.
     */
    // pause 是系统调用, 将任务0 转换为可中断的等待状态, 再执行调度函数
    // 调度函数只要发现任务系统总没有其他任务可以运行时就会切换到任务0,
    // 而不依赖于任务0的状态
    for (;;) pause();
}

static int printf(const char *fmt, ...) {
    va_list args;
    int i;

    va_start(args, fmt);
    // 产生格式化信息并输出到标准设备 stdout(1)
    // 使用 vsprintf 将格式化字符串放入 printbuf 缓冲区, 然后用 write
    // 将缓冲区的内容输出到标准设备
    write(1, printbuf, i = vsprintf(printbuf, fmt, args));
    va_end(args);
    return i;
}

static char *argv_rc[] = {"/bin/sh", NULL};  // 调用执行程序时参数的字符串数组
static char *envp_rc[] = {"HOME=/", NULL};  // 调用执行程序时参数的字符串数组

// 这里的 - 是传递贵shell程序sh的一个标志, 通过识别该标志sh程序
// 会作为更登录shell执行
static char *argv[] = {"-/bin/sh", NULL};
static char *envp[] = {"HOME=/usr/root", NULL};

void init(void) {
    int pid, i;
    // setup 是一个系统调用
    // 读取硬盘参数包括分区表信息, 加载虚拟盘, 安装根文件系统设备
    setup((void *)&drive_info);
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
        execve("/bin/sh", argv_rc,
               envp_rc);  // 将进程自身替换成 /bin/sh 程序,
                          // 参数使用的是 argv_rc envp_rc
        _exit(2);         // 1: 操作未许可 2: 文件/目录不存在
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
            (void)open("/dev/tty0", O_RDWR,
                       0);  // 重新打开 tty0 作为 stdin
            (void)dup(0);   // 复制得到 stdout stderr
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
