
# 工作队列

```bash
$ sudo dmesg
[40880.778390] Initializing the module.
[40880.778527] Thread 0: started and entering sleep.
[40880.778617] Thread 1: started and entering sleep.
[40880.778700] Thread 2: started and entering sleep.
[40887.863018] 10 nodes were added to the queue.
[40887.863028] Thread 1: processing value 9.
[40887.863033] Thread 2: processing value 8.
[40887.863038] Thread 0: processing value 7.
[40887.970923] Thread 2: processing value 6.
[40887.970929] Thread 1: processing value 5.
[40887.970932] Thread 0: processing value 4.
[40888.078933] Thread 0: processing value 3.
[40888.078939] Thread 2: processing value 2.
[40888.078959] Thread 1: processing value 1.
[40888.186934] Thread 2: processing value 0.
```

## 参考

- [深入理解Linux内核的工作队列](https://zhuanlan.zhihu.com/p/688053578)