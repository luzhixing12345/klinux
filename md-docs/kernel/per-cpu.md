
# per-cpu

介绍本文内容之前我们先来快速回顾一下，前文提到当我们在编写多线程操作同一个变量的时候需要额外注意，很容易出现不符合预期的结果，我们来看一个简单的例子。下例我们使用两个线程对一个全局变量 `g_n` 进行自增操作，每个线程循环自增 100000 次，理论上来说最终结果应该为 200000

```c
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define N 100000

int g_n = 0;

void *f(void *arg) {
    for (int i = 0; i < N; i++) {
        g_n++;
    }
    printf("g_n=%d from thread %lu\n", g_n, pthread_self());
    return NULL;
}

int main() {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, f, NULL);
    pthread_create(&t2, NULL, f, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("g_n=%d, should be %d\n", g_n, N * 2);
    return 0;
}
```

但是实际执行结果并没有达到预期，甚至每一次结果都不相同。原因是 `g_n++` 不是原子操作，它其实包括了读、加、写三个步骤，在多个线程中交错执行时会出现数据竞争，导致 g_n 的最终值 小于 200000，[缓存一致性](../arch/cache-coherence.md)机制（如 MESI）不会自动解决数据竞争，它只能保证缓存更新是同步的，但不会协调非原子操作的正确性。

> 如果想要解决这个问题那么可以在执行过程中使用锁([spinlock/mutexlock](./lock.md))或者使用原子操作 [atomic](./atomic.md)，二者的使用场景不同，需要根据需求权衡

```bash
(kamilu) kamilu@vm:~/c-examples$ ./src/thread/thread_add_g_n 
g_n=100083 from thread 140570913715904
g_n=117435 from thread 140570905323200
g_n=117435, should be 200000
(kamilu) kamilu@vm:~/c-examples$ ./src/thread/thread_add_g_n 
g_n=101016 from thread 139949884094144
g_n=116290 from thread 139949875701440
g_n=116290, should be 200000
(kamilu) kamilu@vm:~/c-examples$ ./src/thread/thread_add_g_n 
g_n=107887 from thread 139692458686144
g_n=110036 from thread 139692450293440
g_n=110036, should be 200000
(kamilu) kamilu@vm:~/c-examples$ ./src/thread/thread_add_g_n 
g_n=113134 from thread 140052403365568
g_n=111616 from thread 140052411758272
g_n=113134, should be 200000
```

---

那么还有没有其他方法可以避免多线程竞争导致数据错误的问题呢? 我们来看下面这个例子，几乎没有改动代码，只是把 g_n 的定义前面加上了 `__thread`

```c
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// 使用 __thread 实现线程局部变量（GCC/Clang 支持，等价于 C++ 的 thread_local）
__thread int g_n = 0;

void *f(void *arg) {
    for (int i = 0; i < 100000; i++) {
        g_n++;
    }
    printf("g_n=%d from thread %lu\n", g_n, pthread_self());
    return NULL;
}

int main() {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, f, NULL);
    pthread_create(&t2, NULL, f, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("g_n=%d, should be %d\n", g_n, 100000 * 2);
    return 0;
}
```

惊人的事情发生了！每一个线程都安全的加到了 100000，并且主线程的数据是 0

```bash
(kamilu) kamilu@vm:~/c-examples$ ./src/thread/thread_local 
g_n=100000 from thread 140410397701824
g_n=100000 from thread 140410389309120
g_n=0, should be 200000
```

## 参考

- [Linux 内核 | Per CPU 变量](https://www.dingmos.com/index.php/archives/16/)
- [一张图看懂linux内核中percpu变量的实现](https://cloud.tencent.com/developer/article/1769514)