
# syscall

系统调用其实就是函数调用,只不过调用的是内核态的函数,但是我们知道,用户态是不能随意调用内核态的函数的,所以采用软中断的方式从用户态陷入到内核态。在内核中通过软中断 `0x80` 让操作系统跳转到一个预设好的内核空间地址,它指向了系统调用处理程序(不要和系统调用服务例程混淆),这里指的是在entry.S文件中的system_call函数。就是说,所有的系统调用都会统一跳转到这个地址执行system_call函数,那么system_call函数如何派发它们到各自的服务例程呢?

我们知道每个系统调用都有一个系统调用号。同时,内核中一个有一个 `system_call_table` 数组,它是个函数指针数组,每个函数指针都指向了系统调用的服务例程。这个系统调用号是 `system_call_table` 的下标,用来指明到底要执行哪个系统调用。当int 0x80的软中断执行时,系统调用号会被放进eax寄存器中,system_call函数可以读取eax寄存器获得系统调用号,将其乘以4得到偏移地址,以 `sys_call_table`为基地址,基地址加上偏移地址就是应该执行的系统调用服务例程的地址。

## 添加系统调用

为内核添加一个新的 x86_64 的系统调用, 并不复杂, 大致分为如下几步

### 分配系统调用号

在 `arch/x86/entry/syscalls/syscall_64.tbl` 中添加一个没有使用过的 sysid(456)

```diff
451	common	cachestat		sys_cachestat
452	common	fchmodat2		sys_fchmodat2
453	64	map_shadow_stack	sys_map_shadow_stack
+ 456 common	mycall		    sys_mycall
```

该文件添加的是格式为 `<number> <abi> <name> <entry point>` 其中 abi 可以选 common/64/32, 这里默认 common 即可

后面的 `mycall` 和 `sys_mycall` 分别是用户访问的系统调用名字和内核中实现的系统调用的名字

### 声明系统调用

在 `include/linux/syscalls.h` 中添加如下函数声明:

```c
long sys_mycall(int number);
```

### 添加系统调用函数定义

在 `kernel/sys.c` 中添加如下代码

```c
#define SYSCALL_DEFINE1() // 一个宏, 暂时不去看

SYSCALL_DEFINE1(mycall, int, number)
{
    printk("my syscall has been called with number %d\n", number);
    return 0;
}
```

> 关于 `SYSCALL_DEFINE` 这个系统宏后文介绍

笔者定义了一个简单的输出一句话的系统调用,在这里使用了内核态的printk()函数,输出的信息可以使用dmesg进行查看

重新编译内核

### 测试系统调用

编写一个简单的调用函数, 使用 `syscall` 直接传输自定义的系统调用号(456), 和一个给 number 的参数 100

```c
#include <unistd.h>

#define SYSCALL_ID 456

int main(void)
{
    syscall(SYSCALL_ID, 100);
    return 0;
}
```

编译打包进镜像然后执行, 可以看到成功调用

```bash
~ $ ./bin/a
[   19.161976] my syscall has been called with number 100
```