
# kernel

## 概览

kernel目录下的文件(忽略三个文件夹目录)从功能上可以分为三类: 硬件中断处理程序 系统调用服务处理程序 进程调度通用文件

```bash
├── Makefile
├── asm.s          
├── exit.c
├── fork.c
├── mktime.c
├── panic.c
├── printk.c
├── sched.c
├── signal.c
├── sys.c
├── system_call.s
├── traps.c
└── vsprintf.c
```

## asm.c

从本质上来讲，中断是一种电信号，当设备有某种事件发生时，通过总线把电信号发送给中断控制器

linux当中的中断可以分为两类

- **硬中断**: 由硬件产生的，比如，像磁盘，网卡，键盘，时钟等。每个设备或设备集都有它自己的IRQ（中断请求）。基于IRQ，CPU可以将相应的请求分发到对应的硬件驱动上（注：硬件驱动通常是内核中的一个子程序，而不是一个独立的进程）
- **软中断**: 

![20230323175917](https://raw.githubusercontent.com/learner-lu/picbed/master/20230323175917.png)

## 参考

- [内核中断体系结构](https://blog.csdn.net/qq_42174306/article/details/128635378)
- [Linux硬中断和软中断](https://zhuanlan.zhihu.com/p/85597791)
- [软中断和硬中断](https://www.jianshu.com/p/52a3ee40ea30)