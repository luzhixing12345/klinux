
# io_uring

io_uring 是 Linux 内核提供的一种高性能异步 I/O 框架,旨在解决传统异步 I/O 接口(如 aio)的局限性,并提供更高的性能和更低的延迟.它由 Jens Axboe 开发,从 Linux 5.1 版本开始引入,并逐渐成为 Linux 高性能 I/O 操作的首选方案

在正式介绍 io_uring 之前, 我们先来介绍一下同步IO 和 异步IO

## sync & async

前文 [epoll](./epoll.md) 一文中我们介绍了同步 IO 

### **阻塞 I/O 和 非阻塞 I/O 的同步特性**
1. **阻塞 I/O(Blocking I/O)**
   - 调用 `recv()` 或 `read()` 时,进程会一直等待,直到数据完全从内核拷贝到用户空间,才能返回.
   - 期间,进程处于 **"睡眠状态"**,不会占用 CPU.

2. **非阻塞 I/O(Non-blocking I/O)**
   - 通过 `fcntl(fd, F_SETFL, O_NONBLOCK)` 让 `recv()` 或 `read()` 变成非阻塞,调用时立即返回:
     - **如果数据还未准备好**,立即返回 `-1`,需要主动轮询.
     - **如果数据准备好了**,会立即拷贝数据到用户空间,并返回读取的字节数.
   - 进程仍然**需要不断轮询或借助 `select` / `poll` / `epoll` 监听可读事件**,才能最终完成数据读取.
   - 由于进程仍然需要在用户空间主动处理数据,它依然属于**同步 I/O**.

---

### **真正的异步 I/O(Asynchronous I/O)**
**异步 I/O(AIO)** 指的是 **用户进程发起 I/O 请求后,内核在数据准备好后会主动通知用户进程**,整个过程不会阻塞用户进程,如:
- **POSIX AIO(`aio_read`、`aio_write`)**
- **Linux io_uring(高性能 AIO 机制)**

在 **异步 I/O 模型** 下:
- **用户进程发起 I/O 请求**,但不会等待数据准备,而是立即返回继续执行其他任务.
- **内核在数据准备好后,直接回调通知进程或使用事件机制**,数据已经拷贝到用户缓冲区.

---

### **总结**
| I/O 模型  | 用户态等待方式 | 数据拷贝方式 | 是否同步 |
|-----------|-------------|-------------|----------|
| **阻塞 I/O** | `read()` 阻塞直到数据准备好 | 数据从内核拷贝到用户空间 | ✅ 是 |
| **非阻塞 I/O** | `read()` 立即返回(轮询或 `select`) | 仍然需要进程主动拷贝 | ✅ 是 |
| **异步 I/O** | `aio_read()` 立即返回 | 内核完成数据拷贝后主动通知进程 | ❌ 不是 |

因此,**阻塞 I/O 和 非阻塞 I/O 都是同步 I/O,而真正的异步 I/O 需要内核异步通知进程,进程无需主动等待数据拷贝.**

## 异步 IO 如何实现

异步 I/O (Asynchronous I/O) 是一种 I/O 操作方式,它允许进程**发起一个 I/O 操作后不必等待 I/O 操作完成,而是可以继续执行其他任务,直到 I/O 操作完成时通知进程**.通过这种方式,应用程序可以在等待 I/O 操作完成的同时做其他工作,从而提高系统的效率,尤其是在需要高并发时

在程序中的 IO 操作是相当常见的, 比如读取文件, 写入文件, 从网络获取数据等等. 

```c
#include <liburing.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    struct io_uring ring;
    io_uring_queue_init(32, &ring, 0);

    int fd = open("testfile", O_RDONLY);
    char buffer[1024];

    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    io_uring_prep_read(sqe, fd, buffer, sizeof(buffer), 0);
    io_uring_sqe_set_data(sqe, (void *)buffer);

    io_uring_submit(&ring);

    struct io_uring_cqe *cqe;
    io_uring_wait_cqe(&ring, &cqe);
    if (cqe->res < 0) {
        printf("Error: %d\n", cqe->res);
    } else {
        printf("Read %d bytes: %s\n", cqe->res, (char *)cqe->user_data);
    }

    io_uring_cqe_seen(&ring, cqe);
    close(fd);
    io_uring_queue_exit(&ring);
    return 0;
}
```

## 参考

- [Io uring](https://en.wikipedia.org/wiki/Io_uring)
- [一文让你深入理解Linux异步I/O框架 io_uring(超级详细~)](https://zhuanlan.zhihu.com/p/495766701) 好文