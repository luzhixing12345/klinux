
# rcu

RCU(Read-Copy-Update)是一种用于数据同步的机制,它在Linux内核中发挥着重要作用,特别是在处理并发数据结构时.RCU主要针对的数据对象是链表,其目的是提高数据遍历读取的效率.使用RCU机制时,读取数据不需要对链表进行耗时的加锁操作,从而允许多个线程或进程同时读取该链表,同时允许一个线程对链表进行修改(但修改时需要加锁).

RCU适用于那些需要频繁读取数据而修改操作相对较少的场景.例如,在文件系统中,经常需要查找定位目录,而对目录的修改并不频繁,这时RCU就能发挥其优势.RCU的核心思想是"读-复制-更新":读者可以随意读取,写者在更新数据时需要先复制一份副本,然后在副本上进行修改,最后通过回调机制在所有读取操作完成后替换原数据.

RCU机制通过引入宽限期(Grace period)的概念来确保数据的安全性.在宽限期内,任何删除或更新操作都必须等待,直到所有已经开始的读取操作完成.这样,即使在读取过程中发生了删除或更新,也不会破坏读取操作的完整性.

RCU还提供了一系列的API,如`rcu_read_lock()`和`rcu_read_unlock()`,用来标记RCU读操作的开始和结束.此外,`synchronize_rcu()`函数用于等待宽限期结束,确保所有读取操作都已完成,然后才继续执行删除或释放操作.

RCU的设计思想是明确的,通过新老指针替换的方式来实现免锁方式的共享保护.但是,RCU的使用需要遵守一定的规则,例如在RCU读临界区内不允许发生上下文切换,且调用的函数也不能导致进程睡眠,否则会破坏RCU的设计规则,系统将进入一种不稳定的状态.

总的来说,RCU是一种高效的并发控制机制,尤其适用于读多写少的场景,但它也带来了一定的复杂性,需要开发者对其原理有深刻理解,才能正确地使用它来提升系统性能.

## 参考

- [Linux 内核:RCU机制与使用](https://www.cnblogs.com/schips/p/linux_cru.html)
- [再谈Linux内核中的RCU机制](http://blog.chinaunix.net/uid-23769728-id-3080134.html)
- [最浅显易懂的一篇:RCU机制](https://cloud.tencent.com/developer/article/1521416)
- [rcu 机制简介](https://zhuanlan.zhihu.com/p/113999842)
- [What is RCU? -- "Read, Copy, Update](https://www.kernel.org/doc/html/latest/RCU/whatisRCU.html)
- [RCU基本介绍(一)](https://blog.csdn.net/lsshao/article/details/132472536)
- [linux kernel rcu 读复制更新 并发控制机制 简介](https://blog.csdn.net/whatday/article/details/114474435)
- [深入理解Linux内核 RCU 机制](https://xie.infoq.cn/article/b8445304d95499bae688a9f28)
- [Linux并发与同步(四)RCU](https://carlyleliu.github.io/2021/Linux%E5%B9%B6%E5%8F%91%E4%B8%8E%E5%90%8C%E6%AD%A5%EF%BC%88%E5%9B%9B%EF%BC%89RCU/)
