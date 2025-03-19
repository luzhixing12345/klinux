
# sync_io

在linux系统中,实际上所有的I/O设备都被抽象为了文件这个概念,一切皆文件,磁盘、网络数据、终端,甚至进程间通信工具管道pipe等都被当做文件对待.

在 Unix 和类 Unix 操作系统中,文件描述符(File descriptor,FD)是用于访问文件或者其他 I/O 资源的抽象句柄,例如:管道或者网络套接字.而不同的 I/O 模型会使用不同的方式操作文件描述符.

大多数文件系统的默认IO操作都是缓存IO.在Linux的缓存IO机制中,操作系统会将IO的数据缓存在文件系统的页缓存(page cache).也就是说,数据会**先被拷贝到操作系统内核的缓冲区**中,然后才会从操作系统**内核的缓存区拷贝到应用程序的地址空间**中.这种做法的缺点就是,需要在应用程序地址空间和内核进行多次拷贝,这些拷贝动作所带来的CPU以及内存开销是非常大的.

那为什么不能直接让磁盘控制器把数据送到应用程序的地址空间中呢?最简单的一个原因就是应用程序不能直接操作底层硬件.

总的来说,IO分两阶段:

- 数据准备阶段, 此时从磁盘/网卡等外设通过 DMA 控制器将数据拷贝到内核空间
- 内核空间拷贝回用户进程缓冲区阶段

![20240807124300](https://raw.githubusercontent.com/learner-lu/picbed/master/20240807124300.png)

## 阻塞式I/O模型

阻塞 I/O 是最常见的 I/O 模型,在默认情况下,当我们通过 read 或者 write 等系统调用读写文件或者网络时,应用程序会被阻塞. 进程/线程在从调用 recvfrom 开始到它返回的整段时间内是被阻塞的, 此时**进程阻塞挂起不消耗CPU资源**. 当 recvfrom成功返回后,应用进程/线程开始处理数据报.

如下图所示,当我们执行 read 系统调用时,应用程序会从用户态陷入内核态,内核会检查文件描述符是否可读;当文件描述符中存在数据时,操作系统内核会将准备好的数据拷贝给应用程序并交回控制权.

![20240807120852](https://raw.githubusercontent.com/learner-lu/picbed/master/20240807120852.png)

操作系统中多数的 I/O 操作都是如上所示的阻塞请求,一旦执行 I/O 操作,应用程序会陷入阻塞等待 I/O 操作的结束. 但是显然这种阻塞 IO 的方式只适用并发量小的网络应用开发,不适用并发量大的应用,因为**一个请求IO会阻塞进程**, 每个请求都分配一个处理进程(线程)去响应显然不合理

## 非阻塞式I/O模型

当进程把一个文件描述符设置成非阻塞时,执行 read 和 write 等 I/O 操作会立刻返回.在 C 语言中, 我们可以通过设置 fd 的 flag 为 O_NONBLOCK 将其设置成非阻塞的

```c
int flags = fcntl(fd, F_GETFL, 0);
fcntl(fd, F_SETFL, flags | O_NONBLOCK);
```

在上述代码中,最关键的就是系统调用 fcntl 和参数 `O_NONBLOCK`, fcntl 为我们提供了操作文件描述符的能力,我们可以通过它修改文件描述符的特性.当我们将文件描述符修改成非阻塞后,读写文件会经历以下流程:

![20240807121230](https://raw.githubusercontent.com/learner-lu/picbed/master/20240807121230.png)

**当进程发起IO系统调用后,如果内核缓冲区没有数据,需要到IO设备中读取,进程返回 `EAGAIN` 错误而不会被阻塞;进程发起IO系统调用后,如果内核缓冲区有数据,内核才会把数据返回进程**

应用程序会不断轮询调用 read 直到它的返回值大于 0,这时应用程序就可以对读取操作系统缓冲区中的数据并进行操作.

```c
while (1) {
    int ret = read();
    if (ret != EAGAIN) {
        // do something
    } else {
        // do something else
    }
}
```

但是很明显, 虽然进程使用非阻塞的 I/O 操作时,可以在等待过程中执行其他任务,提高 CPU 的利用率. 但是不断地轮询操作会消耗CPU的资源, 每次由 read 系统调用陷入内核态也会产生性能开销

> [!TIP]
> 例如在一个聊天软件当中, 通信双方建立了 socket 收发信息, 如果使用一个阻塞的 IO 持续监听对方有没有发送信息, 那么此时程序直接进入 sleep 
> 
> 所以可以采用非阻塞的 IO, 只需要每次轮询一下查看对方是否有发送消息, 其余时间依然可以做一些其他的工作(渲染,计算等等)

## IO复用模型

目前支持I/O多路复用的系统调用有 `select,pselect,poll,epoll`.与多进程和多线程技术相比,I/O多路复用技术的最大优势是系统开销小,**系统不必创建进程/线程**,也不必维护这些进程/线程,从而大大减小了系统的开销.

I/O多路复用就是通过一种机制.「多路」: 指的是多个socket网络连接;「复用」: 指的是复用一个线程、使用**一个线程来检查多个文件描述符(Socket)的就绪状态**

多路复用主要有三种技术:select,poll,epoll. epoll是最新的, 也是目前最好的多路复用技术;

select,poll,epoll本质上都是同步I/O,因为他们都需要在读写事件就绪后自己负责进行读写,也就是说这个读写过程是阻塞的,而异步I/O则无需自己负责进行读写,异步I/O的实现会负责把数据从内核拷贝到用户空间

### select

select 使用三个集合来表示希望监控的文件描述符:
- **读集合** (`readfds`): 监控哪些文件描述符可以进行读取操作.
- **写集合** (`writefds`): 监控哪些文件描述符可以进行写入操作.
- **异常集合** (`exceptfds`): 监控哪些文件描述符有异常条件.

```c
int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
```
- `nfds`: 最大文件描述符的值加一(即所有监控文件描述符的最大值 + 1).
- `readfds`: 监控读取的文件描述符集合.
- `writefds`: 监控写入的文件描述符集合.
- `exceptfds`: 监控异常条件的文件描述符集合.
- `timeout`: 等待的最大时间,单位是秒和微秒.如果 `timeout` 为 `NULL`,`select` 将无限期等待,直到至少有一个文件描述符准备好.

`select` 返回准备好的文件描述符的数量,或者在发生错误时返回 -1. 可以通过检查 `readfds`、`writefds` 和 `exceptfds` 来确定哪些文件描述符就绪.

其基本原理如下图所示

![dddf](https://raw.githubusercontent.com/learner-lu/picbed/master/dddf.gif)

select 主要有如下三个问题

1. 每次调用select,都需要把被监控的fds集合从用户态空间拷贝到内核态空间,高并发场景下这样的拷贝会使得消耗的资源是很大的
2. 能监听端口的数量有限,单个进程所能打开的最大连接数由 `FD_SETSIZE` 宏定义, 默认为 1024(通过 ulilmit -n 查看), 最大数量可以查看 `/proc/sys/fs/file-max`, 该数值可以调整

   ```bash
   $ ulimit -n
   1024
   $ cat /proc/sys/fs/file-max
   1609892
   ```

   > 这个数量来说除了超大型项目基本够用, 但是效率会很低

3. 被监控的fds集合中,**只要有一个有数据可读,整个socket集合就会被遍历一次**, 调用sk的poll函数收集可读事
  
   我们不知道事件来的时候,有多少个被监控的socket有数据可读了,于是,只能挨个遍历每个socket来收集可读事件了

其中针对select遗留的三个问题中, 问题 2 是fd限制问题,问题 1 和 3 则是性能问题

select 的文件描述符集合大小受到 `FD_SETSIZE` 的限制,通常在大多数系统上这个值默认是 1024.这意味着,如果你需要监控超过 1024 个文件描述符,select 将无法满足需求.

```c
#define FD_SETSIZE 1024
```

需要在编译时定义一个更大的 FD_SETSIZE 来增加这个限制,但这需要重新编译应用程序,并且不能动态调整

### poll

`poll` 是一种 I/O 多路复用机制,类似于 `select`,但更适合处理大量文件描述符.在 Linux 系统中,`poll` 使用一个 `pollfd` 结构数组来监控多个文件描述符的状态.

`pollfd` 结构包含三个成员:

```c
struct pollfd {
   int   fd;         /* file descriptor */
   short events;     /* requested events */
   short revents;    /* returned events */
};
```

- `fd`: 需要监控的文件描述符.
- `events`: 监控的事件类型.
- `revents`: 实际发生的事件类型.

```c
int poll(struct pollfd *fds, nfds_t nfds, int timeout);
```
- `fds`: 指向 `pollfd` 结构数组的指针.
- `nfds`: 数组中的文件描述符数量.
- `timeout`: 超时时间,单位是毫秒.负值表示无限期等待,0 表示立即返回.

`poll` 返回准备好的文件描述符的数量,或者在发生错误时返回 -1. 可以通过检查 `revents` 字段来确定哪些文件描述符就绪以及发生了哪些事件. 例如我们可以写一个简单的例子来监听来自 TCP socket 和 STDIN 两个 fd 的输入情况

> 示例代码见 [poll.c](https://github.com/luzhixing12345/klinux/blob/main/modules/socket/poll.c) [client.c](https://github.com/luzhixing12345/klinux/blob/main/modules/socket/client.c)

![wmklerjkl](https://raw.githubusercontent.com/learner-lu/picbed/master/wmklerjkl.gif)

### epoll

`epoll` 的设计目的是解决 `select` 和 `poll` 在大规模文件描述符管理中的性能瓶颈.其核心思想是将事件与文件描述符关联,并在内核中维护一个事件表,当文件描述符的状态发生变化时,通过事件通知机制告知用户态进程.

`epoll` 的关键特性包括:

1. **事件驱动**: `epoll` 使用事件通知机制,当文件描述符的状态发生变化时,内核会将其标记为就绪.
2. **边缘触发和水平触发**: `epoll` 支持两种触发模式,边缘触发(Edge Triggered, ET)和水平触发(Level Triggered, LT).
3. **高效的事件管理**: 内核仅在文件描述符状态变化时通知用户态,因此在处理大量文件描述符时性能优越.

`epoll` 的主要 API 包括以下几个函数:

1. `epoll_create1`:
   ```c
   int epoll_create1(int flags);
   ```
   创建一个 `epoll` 实例,返回一个 `epoll` 文件描述符.`flags` 可以是 `EPOLL_CLOEXEC`,表示在执行 `exec` 时关闭文件描述符.

2. `epoll_ctl`:
   ```c
   int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
   ```
   控制 `epoll` 实例上的文件描述符监控.参数解释:
   - `epfd`: 由 `epoll_create1` 返回的 `epoll` 文件描述符.
   - `op`: 操作类型,可以是 `EPOLL_CTL_ADD`(添加)、`EPOLL_CTL_MOD`(修改)或 `EPOLL_CTL_DEL`(删除).
   - `fd`: 需要操作的文件描述符.
   - `event`: 指向 `epoll_event` 结构体的指针,用于描述需要监控的事件.

3. `epoll_wait`:
   ```c
   int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
   ```
   等待事件发生.参数解释:
   - `epfd`: 由 `epoll_create1` 返回的 `epoll` 文件描述符.
   - `events`: 用于返回就绪事件的数组.
   - `maxevents`: `events` 数组的大小.
   - `timeout`: 等待的超时时间(毫秒),-1 表示无限期等待.

示例代码见 [epoll.c](https://github.com/luzhixing12345/klinux/blob/main/modules/socket/epoll.c)

主要步骤包括

4. **创建监听套接字**: 创建一个 TCP 套接字并绑定到指定端口.
5. **创建 `epoll` 实例**: 使用 `epoll_create1` 创建一个 `epoll` 实例.
6. **添加文件描述符到 `epoll` 实例**: 使用 `epoll_ctl` 将监听套接字和标准输入添加到 `epoll` 实例中.
7. **等待和处理事件**: 使用 `epoll_wait` 等待事件发生,并根据不同的文件描述符和事件类型进行处理.

## 参考

- [网络轮询器](https://draveness.me/golang/docs/part3-runtime/ch06-concurrency/golang-netpoller/)
- [IO多路复用_深入浅出理解select、poll、epoll的实现](https://zhuanlan.zhihu.com/p/367591714)