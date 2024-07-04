
# intro

## 内核模块(LKM)

Linux内核模块(Kernel Module)是指可以动态加载到Linux操作系统内核中的独立代码片段.这些模块允许操作系统在**不重新编译内核的情况下扩展和更新功能**,使内核更加灵活和可扩展.

> 从这一点上来说其实已经模糊了微内核和宏内核的概念

LKM同样是ELF格式文件,但是其不能够独立运行,而只能作为内核的一部分存在; 同样的,对于LKM而言,其所处在的**内核空间与用户空间是分开的**,对于通常有着SMAP/SMEP保护的Linux而言,这意味着**LKM并不能够使用libc中的函数,也不能够直接与用户进行交互**

> SMAP(Supervisor Mode Access Prevention)和 SMEP(Supervisor Mode Execution Prevention)是现代CPU(特别是x86架构)提供的两种安全特性,旨在防止内核代码**执行用户空间的代码** 和 **访问用户空间数据**.
> 
> 当SMEP启用时,如果内核代码尝试执行用户空间的代码,CPU会触发一个异常(通常是页错误),从而阻止攻击者利用某些漏洞进行攻击. 当SMAP启用时,如果内核代码尝试直接访问用户空间的内存数据,CPU会触发一个异常.内核可以临时禁用SMAP以允许合法访问用户空间数据,但需要显式地使用内核指令(如`stac`和`clac`)进行控制.
>
> SMAP/SMEP 通常在现代操作系统中默认启用.可以通过检查CPU特性来确认支持情况.
> 在Linux中,可以使用以下命令检查SMEP的支持情况:
> 
> ```bash
> dmesg | grep -i smep
> dmesg | grep -i smap
> ```


虽然我们同样能够使用C语言编写LKM,但是作为内核的一部分,LKM编程在一定意义上便是内核编程, 内核版本的每次变化意味着某些函数名也会相应地发生变化,因此LKM编程与内核版本密切相关

### 简单的模块

我们来编写这样一个简单的内核模块,其功能是在载入/卸载时会在内核缓冲区打印字符串:

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static int __init kernel_module_init(void)
{
    printk("<1>Hello the Linux kernel world!\n");
    return 0;
}

static void __exit kernel_module_exit(void)
{
    printk("<1>Good bye the Linux kernel world! See you again!\n");
}

module_init(kernel_module_init);
module_exit(kernel_module_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("kkk");
```

**头文件**

- `linux/module.h`:对于LKM而言这是必须包含的一个头文件
- `linux/kernel.h`:载入内核相关信息
- `linux/init.h`:包含着一些有用的宏
通常情况下,这三个头文件对于内核模块编程都是不可或缺的

**入口点/出口点**

内核模块的初始化函数在编译时通过 `module_init()` 定义,在内核模块被载入时会调用所定义的函数,这里我们将初始化函数设定为 `kernel_module_init`

内核模块的卸载函数在编译时通过 `module_exit()` 定义,在内核模块被卸载时会调用所定义的函数,这里我们将卸载函数定义为 `kernel_module_exit`

**其他…**

- `__init` 与 `__exit` 宏:用来显式标识内核模块出入口函数
- `MODULE_AUTHOR() & MODULE_LICENSE()`:声明内核作者与发行所用许可证

与一般的可执行文件所不同的是,我们应当使用 Makefile 来构建一个内核模块,并使用 Kbuild 说明编译规则

首先创建一个 `Kbuild` 文件,写入如下内容

```txt
MODULE_NAME ?= hellokernel

obj-m += $(MODULE_NAME).o

$(MODULE_NAME)-y += main.o
```


## 参考

- [【OS.0x01】Linux Kernel II:内核简易食用指北 ](https://arttnba3.cn/2021/02/21/OS-0X01-LINUX-KERNEL-PART-II/)