
# 驱动程序

输入输出(I/O)设备模型本质上是由许多寄存器构成的.同样地,我们可以将内存,如DDR,视为由大量寄存器组成的.当我们执行物理内存的读写操作,比如访问0x123456这个地址时,实际上是从内存管理器(MMU)的寄存器中读取或写入数据, 这个过程可以简化为从寄存器中获取数据并发送给中央处理器(CPU),或者将数据从CPU写入寄存器.

因此,I/O设备在操作方式上与内存非常相似.自然而然地,我们会将I/O设备抽象为具有控制寄存器(包括命令寄存器command和数据寄存器data)的计算器.一旦这些设备被**映射到与CPU相同的地址空间**,CPU就可以使用load和store指令直接访问这些设备.例如,CPU可以查询设备是否准备就绪,如果设备ready,就可以将字节写入设备.

设备驱动程序是一种特殊的软件,它充当操作系统和硬件设备之间的桥梁.它的主要功能是将操作系统发出的通用指令翻译成特定硬件设备能够理解和执行的命令.这样,操作系统就能够控制和管理各种硬件设备,而不需要为每一种硬件都编写特定的控制代码.

## 驱动层设计

为了使软件能够访问I/O设备,我们可以考虑将控制寄存器直接暴露给应用程序.理论上,操作系统可以提供一个系统调用,比如说 `dev_write`, 它可以允许应用程序直接发送命令给设备.

```c
dev_write(reg, data);
```

然而这种直接的控制方式并不是最佳实践,尤其是在操作系统需要有效管理资源并确保应用程序顺畅使用这些资源的情况下.

操作系统的目标之一是**确保应用程序在使用资源时不会相互干扰**.例如,应用程序在使用CPU时,应该感觉不到自己被中断或抢占.它们应该感觉自己独占CPU,持续不断地执行指令.同样地,内存的虚拟化技术让每个进程都拥有自己的地址空间,其中包含了可读、可写、可执行的区域,而无需关心物理内存的具体布局.

因此,操作系统在管理I/O设备时,**不应该简单地将底层设备寄存器暴露给程序, 更好的方法是将设备抽象成操作系统中的对象**.例如,字符终端可以被视为操作系统的一个对象,磁盘也是如此.这样,应用程序在访问设备时,可以使用统一的接口,而无需关心底层的具体实现.

对于I/O设备,其核心功能通常可以归结为输入(input)和输出(output).例如,打印机接收打印任务并输出打印内容,字符终端允许用户输入数据并输出显示信息,而磁盘则作为一个巨大的字节数组,支持数据的读取和写入.因此,操作系统为I/O设备定义的基本操作也主要围绕这两个功能展开.在Linux等操作系统中,最基本的I/O操作是read和write.这两个操作允许应用程序从设备读取数据(read)或向设备写入数据(write).这些操作通常可以指定一个偏移量,以便在设备上的正确位置进行数据的读取或写入.

```c
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
```

我们可以得到两类 IO 设备的抽象: **字节流**(byte stream)类型的, 例如终端, 网络套接字; **字节数组**(byte array), 例如磁盘

除了基本的读写操作,I/O设备还可能需要一些控制功能.例如,我们可能需要查询设备的状态、配置设备的工作模式或获取设备的特定信息.为了实现这些控制功能,操作系统提供了I/O控制(I/O control)操作.通过I/O控制操作,应用程序可以发送特定的控制命令给设备,或者查询设备的状态和参数.

```c
// 读取/设置设备的状态
int ioctl(int fd, unsigned long request, ...);
```

在操作系统中,设备被抽象为文件系统中的一个**对象**,这意味着我们可以使用文件描述符(fd)来引用和操作设备.文件描述符本质上是一个指针,指向操作系统中的特定对象,允许我们对其进行各种操作.这些操作与文件操作类似,因为操作系统遵循"**一切皆文件**"(everything is a file)的原则.read和write操作允许我们从设备读取数据或向设备发送数据,而I/O control操作则用于执行对设备的特定控制任务,例如查询设备状态或修改设备配置.

## 设备驱动程序

设备驱动程序是计算机系统中的一个重要组成部分,它起着桥梁的作用,连接操作系统和硬件设备.当我们将一个新的硬件设备接入电脑时,设备可能无法立即正常工作,特别是对于一些功能复杂的设备,如显卡.这是因为操作系统可能没有内置与该设备完全兼容的驱动程序.

> 在这种情况下,操作系统会尝试自动搜索并安装适合该硬件的驱动程序.例如,在Windows系统中,当我们插入一块显卡,系统会自动在互联网上寻找相应的驱动程序, 或者前往 NVIDIA 官网下载对应的驱动安装程序, 一旦驱动程序安装完成,显卡的性能会得到显著提升,例如,从低分辨率显示升级到高清分辨率显示.

设备驱动程序是一段特殊的代码,由设备制造商提供,它详细定义了设备中每个寄存器的功能和如何操作这些寄存器.驱动程序的作用是**将操作系统发出的通用命令,如read、write和I/O control,翻译成硬件设备能够理解的特定操作**.这就像是设备和操作系统之间的翻译者.例如,当我们执行一个read操作时,设备驱动程序会根据设备的技术规范,将这个操作转换为对控制寄存器的特定设置,然后等待状态寄存器的某个位变化,以确定可以进行数据读取.这个过程是按照设备制造商设定的协议进行的.

在Linux系统中,设备驱动程序的概念尤为明显.系统中存在一个名为`/dev`的目录,其中包含了许多设备文件.这些设备文件代表了系统中的各种硬件设备,通过这些设备文件,应用程序可以进行I/O操作.设备驱动程序负责处理对这些设备文件的操作请求,并将其转换为实际的硬件操作.

在操作系统中,遵循"一切皆文件"(everything is a file)的原则,**设备也被当作文件来处理**.通过`ls -l /dev`命令,我们可以查看系统中的设备文件.设备文件主要分为两种类型:**字符设备**(character device)和**块设备**(block device).

- 字符设备是以字节流的形式进行数据传输的设备.例如,键盘和鼠标都是字符设备,它们以字符为单位与系统交互.在`/dev`目录下,字符设备文件以字母'c'开头,表示它们是按字符进行操作的.
- 块设备则以数据块为单位进行数据传输,通常用于存储设备,如硬盘和SSD.在`/dev`目录下,块设备文件以字母'b'开头,表示它们按块进行操作.

在Linux系统中,`/dev`目录包含了许多虚拟设备文件,这些文件代表了系统中的各种硬件设备和特殊文件,如`stdin`、`stdout`和`stderr`,分别代表标准输入、标准输出和标准错误输出.这些设备允许进程与终端进行交互.例如,我们可以通过`/dev/stderr`将错误信息重定向到其他地方

1. **`/dev/null`**: 这是一个特殊的文件,它丢弃所有写入其中的数据,同时返回写入操作成功完成的信号.它经常用于丢弃不需要的输出,例如:`command > /dev/null`.

2. **`/dev/zero`**: 这个设备文件提供了一个无尽的零字节流.读取`/dev/zero`会返回连续的零字节,这在需要生成大量零字节数据时非常有用.

3. **`/dev/random` 和 `/dev/urandom`**: 这两个设备文件用于生成随机数.`/dev/random`提供高质量的随机数,它会等待收集足够的熵来生成随机数,而`/dev/urandom`则提供不那么高质量但速度更快的随机数.

4. **`/dev/tty`**: 这个设备文件代表当前终端会话.它可以用来读取或写入当前终端的输入输出.

5. **`/dev/stdin`、`/dev/stdout` 和 `/dev/stderr`**: 这些设备文件分别代表标准输入、标准输出和标准错误流.它们允许程序直接与这些流交互.

6. **`/dev/ptmx`**: 这是一个伪终端主设备文件,用于创建伪终端对,允许非特权用户模拟终端会话.

7. **`/dev/full`**: 当系统内存不足,无法创建新文件时,写入这个设备会使得写操作等待,直到有足够的空间.

8. **`/dev/loop*`**: 这些设备文件代表循环设备,它们允许将文件作为块设备进行挂载.

9. **`/dev/sda*`、`/dev/sdb*` 等**: 这些是块设备的设备文件,代表系统中的硬盘和分区.例如,`/dev/sda1` 可能代表第一个硬盘的第一个分区.

10. **`/dev/ttyS*`**: 这些是串行端口设备文件,用于串行通信.

11. **`/dev/fb*`**: 这些设备文件代表帧缓冲设备,用于直接操作显示设备.

这些虚拟设备文件是Linux系统中的重要组成部分,它们提供了与硬件设备交互的标准化接口,简化了硬件访问和管理工作,同时也为用户和开发者提供了丰富的操作选项.通过这些设备文件,Linux系统能够灵活地处理各种输入输出任务.

## 一切皆文件的问题

设备驱动程序在概念上是简单的,其基本职责是实现数据的读取和写入.无论是高性能的NVMe磁盘还是日常使用的键盘和鼠标,它们都通过驱动程序来进行数据的输入输出.然而,实际的设备驱动程序开发远比这复杂得多.

现代的硬件设备通常不仅仅提供基本的读写功能,它们还具备各种高级特性和配置选项.例如,现代键盘可能带有背光灯效、多媒体控制键、宏命令键等.这些高级功能需要设备驱动程序来支持和控制.对于键盘的背光灯效,驱动程序需要实现特定的I/O控制操作来管理这些灯光的显示效果.打印机的打印质量/进纸/双面控制、卡纸、清洁、自动装订; 磁盘的健康状况、缓存控制 ...

所有和设备控制相关的功能都会集中到 `ioctl` 中, 每个设备有它自己的 ioctl, 设备驱动程序的复杂性来源于它需要支持设备的所有功能,包括基本的读写操作和高级的配置管理.Linux系统的设计哲学"一切皆文件"**将这些复杂性隐藏在文件系统之后**,使得开发者和用户可以通过标准的文件操作接口来与设备交互.**这种设计虽然在概念上简洁优雅,但实际上增加了文件系统和驱动程序的实现复杂度**.每个设备的独特功能和行为都需要在驱动程序中得到妥善处理,以确保系统的稳定性和可用性.

水面下有冰山

## Linux 设备驱动

编写设备驱动程序在Linux操作系统中是一个涉及创建操作系统对象的过程,这个对象支持标准的文件操作,如read、write和ioctl(input, output, control).这些操作使得设备能够与操作系统以及其他程序进行交互.设备驱动程序的核心是实现一组称为 `file operations`的结构体,该结构体定义了设备如何响应文件系统API的调用.

```c
struct file_operations {
	struct module *owner;
	loff_t (*llseek) (struct file *, loff_t, int);
	ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
	ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
	ssize_t (*read_iter) (struct kiocb *, struct iov_iter *);
	ssize_t (*write_iter) (struct kiocb *, struct iov_iter *);
	int (*iopoll)(struct kiocb *kiocb, bool spin);
	int (*iterate) (struct file *, struct dir_context *);
	int (*iterate_shared) (struct file *, struct dir_context *);
	__poll_t (*poll) (struct file *, struct poll_table_struct *);
	long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
	long (*compat_ioctl) (struct file *, unsigned int, unsigned long);
	int (*mmap) (struct file *, struct vm_area_struct *);
	unsigned long mmap_supported_flags;
	int (*open) (struct inode *, struct file *);
	int (*flush) (struct file *, fl_owner_t id);
	int (*release) (struct inode *, struct file *);
	int (*fsync) (struct file *, loff_t, loff_t, int datasync);
	int (*fasync) (int, struct file *, int);
	int (*lock) (struct file *, int, struct file_lock *);
    // ...
} __randomize_layout;
```

在Linux内核中,设备可以是物理的硬件组件,也可以是虚拟的软件构造.无论是物理设备还是虚拟设备,它们都可以通过file operations来定义其行为.例如,一个虚拟设备可能没有实际的硬件对应物,但它可以提供特定的服务或功能,如网络接口或伪终端.

因此如果想要编写一个内核模块来创建一个新的虚拟设备,只需要定义这个模块的file operations,并在其中指定如何处理对该设备的读写请求和控制请求, **完成函数实现并将其绑定到结构体的函数指针**. 这样的内核模块被加载到操作系统中,你的虚拟设备就会像任何其他文件一样出现在文件系统中,可以被应用程序通过标准的文件操作API进行访问.

> 细心的同学可能会看到有两个 ioctl
>
> ```c
> long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
> long (*compat_ioctl) (struct file *, unsigned int, unsigned long);
> ```
>
> unlocked_ioctl: BKL (Big Kernel Lock) 时代的遗产
> - 单处理器时代只有 ioctl
> - 之后引入了 BKL, ioctl 执行时默认持有 BKL
> - (2.6.11) 高性能的驱动可以通过 unlocked_ioctl 避免锁
> - (2.6.36) ioctl 从 struct file_operations 中移除
> compact_ioctl: 机器字长的兼容性
> - 32-bit 程序在 64-bit 系统上可以 ioctl
> - 此时应用程序和操作系统对 ioctl 数据结构的解读可能不同 (tty)
>
> 简单来说新开发的设备驱动程序通常推荐使用 `compact_ioctl` 函数来代替 `unlocked_ioctl` 函数.

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
- [设备驱动程序与文件系统 (Linux 设备驱动;目录管理 API) [南京大学2023操作系统-P27] (蒋炎岩)](https://www.bilibili.com/video/BV1m24y1A7Fi/)