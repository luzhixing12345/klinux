
# signal

信号机制是UNIX系统最古老的机制之一,它不仅是内核处理程序在运行时发生错误的方式,还是终端管理进程的方式,并且还是一种进程间通信机制。它普遍到当你像往常一样使用 ctrl+c 终止了一个程序, 你可能甚至都没有意识到发生了一次进程间通信。

本文我们从这三方面入手详细介绍一下信号 signal 的机制

1. 信号是怎么产生的,或者说是谁发送的?
2. 信号是怎么投递到进程或者线程的?
3. 信号是怎么处理的?

## 信号的产生

某一天我们在路口等待红绿灯,此时我们发现红灯变成了绿灯,但是前车依然没有动。此时我们并不会给他发短信或是下车走到车前敲敲玻璃告诉他已经绿灯了,而是会按下喇叭。前车司机听到喇叭声会猛然惊醒,然后检查发现已经变成绿灯,赶紧启动了车子继续行驶了

![Clipboard_Screenshot_1755956059](https://raw.githubusercontent.com/learner-lu/picbed/master/Clipboard_Screenshot_1755956059.png)

这是一个相当形象的例子,事实上这也暗示了信号作为进程间通信时的使用场景,即利用内核的信号机制告知其他进程来完成异步通信。不需要双方协商好数据格式以及收发处理,而是用信号机制(喇叭声)心照不宣的完成信息的传递(信号灯变绿)和处理(可以行驶).

### 信号类型

信号的产生方式也就是发送方有三种,分别是

- **终端发送**,比如我们在终端里输入Ctrl+C快捷键时,终端会给当前进程发送SIGINT信号
- **内核发送**,比如进程非法访问内存,在异常处理中就会给当前线程发送SIGSEGV信号,也就是报错看到 segmentation fault.
- **进程发送**,也就是一个进程给另一个进程发送或者是进程自己给自己发送。这里有很多接口函数可以选择,有的可以发给线程,有的可以发给进程,有的可以发给进程组甚至会话组。

> 终端发送属于进程发送的一种特例,因为shell本身是一个较为特殊的进程,因此单独做为一类

进程收到的信号可以来自于其他进程。但不是所有的进程都可以向其他任意一个进程发送信号,只有具有root权限的super user才可以这么做,对于普通user的进程,只能向属于同一user的进程发送信号,而用户对于内核线程发送的信号内核线程是不会响应的

在终端输入 `kill -l` 即可查看到所有的信号类型,一共 64 个。不难发现前 1-31 号信号有各自的缩写名字,34-64 号则使用 SIGRTMIN+x 和 SIGRTMAX-x 为代号。

```txt
$ kill -l
 1) SIGHUP       2) SIGINT       3) SIGQUIT      4) SIGILL       5) SIGTRAP
 2) SIGABRT      7) SIGBUS       8) SIGFPE       9) SIGKILL     10) SIGUSR1
1)  SIGSEGV     12) SIGUSR2     13) SIGPIPE     14) SIGALRM     15) SIGTERM
2)  SIGSTKFLT   17) SIGCHLD     18) SIGCONT     19) SIGSTOP     20) SIGTSTP
3)  SIGTTIN     22) SIGTTOU     23) SIGURG      24) SIGXCPU     25) SIGXFSZ
4)  SIGVTALRM   27) SIGPROF     28) SIGWINCH    29) SIGIO       30) SIGPWR
5)  SIGSYS      34) SIGRTMIN    35) SIGRTMIN+1  36) SIGRTMIN+2  37) SIGRTMIN+3
6)  SIGRTMIN+4  39) SIGRTMIN+5  40) SIGRTMIN+6  41) SIGRTMIN+7  42) SIGRTMIN+8
7)  SIGRTMIN+9  44) SIGRTMIN+10 45) SIGRTMIN+11 46) SIGRTMIN+12 47) SIGRTMIN+13
8)  SIGRTMIN+14 49) SIGRTMIN+15 50) SIGRTMAX-14 51) SIGRTMAX-13 52) SIGRTMAX-12
9)  SIGRTMAX-11 54) SIGRTMAX-10 55) SIGRTMAX-9  56) SIGRTMAX-8  57) SIGRTMAX-7
10) SIGRTMAX-6  59) SIGRTMAX-5  60) SIGRTMAX-4  61) SIGRTMAX-3  62) SIGRTMAX-2
11) SIGRTMAX-1  64) SIGRTMAX
```

最初设计时 UNIX系统只有1-31总共31个信号,这些信号每个都有特殊的含义和特定的用法,是 UNIX 最早期规定的**标准信号**.这些标准信号的实现有一个特点,它们是用**bit flag**实现的。这就会导致当一个信号还在待决的时候,又来了一个同样的信号,再次设置bit位是没有意义的,所以就会**丢失再次收到的信号**,因此 1-31 号信号也被称为**不可靠信号**

一个简单的示例, 执行下面的程序然后连续三次使用 KILL 发送信号, 第三次的信号不会响应

```c
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

int counter = 0;

void handler(int sig) {
    counter++;
    printf("Received signal %d, counter = %d\n", sig, counter);
    sleep(3); // 模拟长时间处理
}

int main() {
    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("Process PID = %d\n", getpid());
    printf("Send `kill -USR1 %d` quickly in another terminal.\n", getpid());
    while (1) {
        pause(); // 等待信号
    }

    return 0;
}
```

为了解决这个问题,后来POSIX规定增加32-64这33个信号作为**实时信号**(RT的由来),并规定实时信号不能丢失,要用队列来实现。这样后面再次到来的信号不会被丢弃而是直接放入队列等待依次处理。因此 32-64 的信号也被称为**可靠信号**

那么标准信号都有哪些呢?

| Number | Name      | Default Action          | Corresponding event (in Chinese)            |
| ------ | --------- | ----------------------- | ------------------------------------------- |
| 0      |           |                         | 测试进程/线程是否存在                       |
| 1      | SIGHUP    | Terminate               | 终端线路挂起,通常由终端或内核发送           |
| 2      | SIGINT    | Terminate               | 键盘中断,通常由用户按下Ctrl+C触发           |
| 3      | SIGQUIT   | Terminate               | 键盘退出,通常由用户按下Ctrl+\触发           |
| 4      | SIGILL    | Terminate and dump core | 非法指令,通常由CPU异常触发                  |
| 5      | SIGTRAP   | Terminate and dump core | 跟踪陷阱,通常由调试器或程序触发             |
| 6      | SIGABRT   | Terminate and dump core | 程序调用abort函数,通常由进程自身触发        |
| 7      | SIGBUS    | Terminate and dump core | 总线错误,通常由硬件或内存访问错误触发       |
| 8      | SIGFPE    | Terminate and dump core | 浮点异常,通常由程序中的浮点错误触发         |
| 9      | SIGKILL   | Terminate               | 强制终止程序,通常由进程或系统管理员发送     |
| 10     | SIGUSR1   | Terminate               | 用户定义信号1,通常由用户或进程发送          |
| 11     | SIGSEGV   | Terminate and dump core | 无效内存引用,通常由进程访问无效内存触发     |
| 12     | SIGUSR2   | Terminate               | 用户定义信号2,通常由用户或进程发送          |
| 13     | SIGPIPE   | Terminate               | 向没有读取者的管道写入数据,通常由进程发送   |
| 14     | SIGALRM   | Terminate               | 定时器信号,通常由alarm函数触发              |
| 15     | SIGTERM   | Terminate               | 软件终止信号,通常由程序或系统管理员发送     |
| 16     | SIGSTKFLT | Terminate               | 协处理器栈错误,通常由硬件或内核发送         |
| 17     | SIGCHLD   | Ignore                  | 子进程停止或终止,通常由内核发送             |
| 18     | SIGCONT   | Ignore                  | 恢复停止的进程,通常由进程或终端发送         |
| 19     | SIGSTOP   | Stop until SIGCONT      | 停止进程,通常由进程或终端发送               |
| 20     | SIGTSTP   | Stop until SIGCONT      | 终端发出的停止信号,通常由用户按下Ctrl+Z触发 |
| 21     | SIGTTIN   | Stop until SIGCONT      | 背景进程从终端读取数据时,通常由终端发送     |
| 22     | SIGTTOU   | Stop until SIGCONT      | 背景进程向终端写入数据时,通常由终端发送     |
| 23     | SIGURG    | Ignore                  | 套接字上的紧急情况,通常由内核发送           |
| 24     | SIGXCPU   | Terminate               | 超过CPU时间限制,通常由内核发送              |
| 25     | SIGXFSZ   | Terminate               | 文件大小超过限制,通常由内核发送             |
| 26     | SIGVTALRM | Terminate               | 虚拟定时器到期,通常由程序或内核发送         |
| 27     | SIGPROF   | Terminate               | 性能分析定时器到期,通常由程序或内核发送     |
| 28     | SIGWINCH  | Ignore                  | 窗口大小改变,通常由终端发送                 |
| 29     | SIGIO     | Terminate               | I/O操作变得可能,通常由内核发送              |
| 30     | SIGPWR    | Terminate               | 电源故障,通常由内核发送                     |
| 31     | SIGSYS    | Terminate and Coredump  | 系统调用错误,通常由内核发送                 |

上表记录了所有标准信号的含义。其中部分信号比较常见,比如中断程序的 SIGINT,强制杀死程序的 SIGKILL,暂停进程的 SIGTSTP 和继续进程的 SIGCONT.除此之外还有一些不那么常见但是很有意思的 signal

- 0: 0号信号并不存在,实际上他的作用是判断一个进程是否存在,因为发送信号0并不会真的投递给进程或者线程。检测流程会检测发送者是否有权限发送、进程是否存在,如果遇到问题就返回错误值。所以发送信号0可以用作检测进程是否存在的方法。
- **SIGSEGV**: 无须多言,访问无效地址触发的 segmentation fault 的信号
- **SIGALRM**: 如果我希望设定一个计时器 1s 之后提醒我一下,那么我可以使用 alarm(1),实际上他就是开启了一个定时器,并在 1s 之后发送一个 SIGALRM 信号。如果我希望每秒钟都收到信号则可以使用 setitimer,下面是一个简单的例子,每隔1s打印一下。比较适合做一些即时输出的统计。

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <unistd.h>
  #include <signal.h>
  #include <sys/time.h>

  void handle_alarm(int sig) {
      printf("SIGALRM received: Process scheduled\n");
  }

  int main() {
      signal(SIGALRM, handle_alarm);

      // 设置定时器,每秒触发一次 SIGALRM 信号
      struct itimerval timer;
      timer.it_value.tv_sec = 1;   // 初始延迟 1 秒
      timer.it_value.tv_usec = 0;
      timer.it_interval.tv_sec = 1;  // 定时器每秒触发一次
      timer.it_interval.tv_usec = 0;

      if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
          perror("setitimer failed");
          exit(1);
      }

      printf("SIGALRM will trigger every 1 second...\n");
      while (1) {
          sleep(1);
      }
      return 0;
  }
  ```

  > 当然,如果想实现同样的功能也可以创建一个线程,然后运行后sleep(1),效果相同

- **SIGWINCH**: 有没有想过类似 vim/man/less 等终端显示的软件是如何做到宽度自适应的?是的,当你调整终端窗口时会向前台进程发送 SIGWINCH 信号。你可以轻松的捕获该信号并根据长宽重新绘制,试一下吧

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <unistd.h>
  #include <signal.h>
  #include <sys/ioctl.h>
  #include <termios.h>

  void handle_winch(int sig) {
      struct winsize ws;
      if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
          perror("ioctl");
          return;
      }
      printf("\nWindow size changed: %d rows, %d columns\n", ws.ws_row, ws.ws_col);
  }

  int main() {
      if (signal(SIGWINCH, handle_winch) == SIG_ERR) {
          perror("Unable to catch SIGWINCH");
          exit(1);
      }

      printf("Press Ctrl+C to exit. Resize the terminal window to trigger SIGWINCH...\n");
      while (1) {
          sleep(1);
      }
      return 0;
  }
  ```

- **SIGABRT**: POSIX规范要求abort函数执行完成之后,进程一定要被杀死。当库程序发现程序出现了不可挽回的错误,就会调用函数abort给当前线程发送信号SIGABRT

### 信号属性

标准信号都有默认的处理方式,也就是说,如果目标进程不注册某个信号的处理函数,那么当它收到这个信号后,就会执行信号默认的操作。这些标准信号有三个属性,**是否可阻塞,是否可忽略,是否可捕获**

- 阻塞,指暂时屏蔽一个信号,即当我在处理这个信号的时候,我希望暂时不去处理某些信号。当然这种屏蔽是暂时的,当阻塞行为结束依然需要响应这些被阻塞了的信号。
- 忽略,指不去响应这些信号,阻塞是暂时不处理,而忽略其实也是一种处理,相当于是空处理
- 捕获,我们可以通过一些接口来设置信号处理函数handler来处理信号,这个行为叫做捕获

关于某个信号是否可阻塞/忽略/捕获可以参考下图

![Clipboard_Screenshot_1756043225](https://raw.githubusercontent.com/learner-lu/picbed/master/Clipboard_Screenshot_1756043225.png)

可以看到大部分信号都是可以捕获的,让它们可以捕获的原因是因为这样可以让进程知道自己出错的原因,让进程可以在临死之前可以做一些记录工作,为程序员解BUG多提供一些信息。但不是所有信号的默认行为都可以被目标进程更改,比如SIGKILL就不可以,只能直接受死,不容许片刻挣扎,相当于"斩立决".试想一下,如果进程可以自行设计所有信号的处理函数,那操作系统可能就无法控制某些进程。不过面对大杀器SIGKILL,有一个进程是可以被豁免的,那就是init进程。Init进程作为开国功臣,位高权重,内核是不允许它被kill掉的。

> CSAPP [ShellLab](https://luzhixing12345.github.io/csapplab/articles/md-docs/09-ShellLab/) 中会要求实现一个简单的 shell,其中就包含了对信号的处理操作,感兴趣的读者可以尝试完成一下


## 参考

- [深入理解Linux信号机制](https://mp.weixin.qq.com/s?__biz=Mzg2OTc0ODAzMw==&mid=2247509281&idx=1&sn=6d59398ac8e31476a7a76f364cf0ace4&chksm=ce9ab90ff9ed3019913db4752bb4d8890a6ce9ecea3451ccf50f00e07bb2ccfb797b349fa287&scene=178&cur_album_id=2519398872503353344#rd)
- [Stop Killing Processes! Make Ctrl+C Meow Instead… (with Signals)](https://www.youtube.com/watch?v=m6WXrC9Mxzo)
- [Linux中的信号处理机制 [一]](https://zhuanlan.zhihu.com/p/77598393)
- [Linux中的信号处理机制 [二]](https://zhuanlan.zhihu.com/p/79062142)
- [Linux中的信号处理机制 [三]](https://zhuanlan.zhihu.com/p/77627175)
- [Linux中的信号处理机制 [四]](https://zhuanlan.zhihu.com/p/78653866)
- [信号的基本概念](http://akaedu.github.io/book/ch33s01.html)