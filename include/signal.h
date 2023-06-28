#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <sys/types.h>

typedef int sig_atomic_t;       // 信号原子操作类型
typedef unsigned int sigset_t;  // 信号集类型

#define _NSIG 32  // 信号种类32种
#define NSIG  _NSIG

// POSIX.1 要求的全部 20 个信号
#define SIGHUP    1   // hang up 挂断控制终端或进程
#define SIGINT    2   // interrupt 来自键盘的中断
#define SIGQUIT   3   // quit 来自键盘的退出
#define SIGILL    4   // illeagle 非法指令
#define SIGTRAP   5   // trap 跟踪断点
#define SIGABRT   6   // abort 异常结束
#define SIGIOT    6   // io trap 异常结束(同上)
#define SIGUNUSED 7   // unused 没有使用
#define SIGFPE    8   // FPE 协处理器出错
#define SIGKILL   9   // kill 强迫进程终止
#define SIGUSR1   10  // user1 用户信号1, 进程可使用
#define SIGSEGV   11  // segment violation 无效内存引用
#define SIGUSR2   12  // user2 用户信号2, 进程可使用
#define SIGPIPE   13  // pipe 管道写出错, 无读者
#define SIGALRM   14  // alarm 实时定时器报警
#define SIGTERM   15  // terminate 进程终止
#define SIGSTKFLT 16  // stack fault 栈出错
#define SIGCHLD   17  // child 子进程停止或被终止
#define SIGCONT   18  // continue 恢复进程继续执行
#define SIGSTOP   19  // stop 停止进程的执行
#define SIGTSTP   20  // tty stop 发出停止进程
#define SIGTTIN   21  // tty in 后台进程请求输入
#define SIGTTOU   22  // tty out 后台进程请求输出

/* Ok, I haven't implemented sigactions, but trying to keep headers POSIX */
#define SA_NOCLDSTOP 1
#define SA_NOMASK    0x40000000
#define SA_ONESHOT   0x80000000

#define SIG_BLOCK    0 /* for blocking signals */
#define SIG_UNBLOCK  1 /* for unblocking signals */
#define SIG_SETMASK  2 /* for setting the signal mask */

#define SIG_DFL      ((void (*)(int))0) /* default signal handling */
#define SIG_IGN      ((void (*)(int))1) /* ignore signal */

struct sigaction {
    void (*sa_handler)(int);
    sigset_t sa_mask;
    int sa_flags;
    void (*sa_restorer)(void);
};

void (*signal(int _sig, void (*_func)(int)))(int);
int raise(int sig);
int kill(pid_t pid, int sig);
int sigaddset(sigset_t *mask, int signo);
int sigdelset(sigset_t *mask, int signo);
int sigemptyset(sigset_t *mask);
int sigfillset(sigset_t *mask);
int sigismember(sigset_t *mask, int signo); /* 1 - is, 0 - not, -1 error */
int sigpending(sigset_t *set);
int sigprocmask(int how, sigset_t *set, sigset_t *oldset);
int sigsuspend(sigset_t *sigmask);
int sigaction(int sig, struct sigaction *act, struct sigaction *oldact);

#endif /* _SIGNAL_H */
