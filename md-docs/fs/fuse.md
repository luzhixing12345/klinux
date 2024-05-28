
# FUSE介绍

## 什么是FUSE?

FUSE(**F**ilesystem in **Use**rspace)是一个允许非特权用户在**用户空间**创建自己的文件系统的工具. 

在fuse出现以前,Linux中的文件系统都是完全实现在内核态, 而内核下定制开发文件系统难度较高. 除此之外用户空间下调试工具/第三方库丰富,出问题不会导致内核崩溃,开发难度相对较低. 简而言之, fuse 有如下三个主要优势:

- 它提供了一种机制,使得开发者可以在用户空间, 有利于**编写和调试**.
- 开发者可以利用**多种编程语言**和**第三方库**来创建文件系统,而不需要深入了解Linux内核的复杂性.
- 编写FUSE文件系统时,只需要内核加载了fuse内核模块即可,**不需要重新编译内核**.

## FUSE的工作原理

FUSE 由内核部分和用户级守护程序组成.内核部分作为 Linux 内核模块实现,加载时会向 Linux 的 VFS 注册文件系统驱动程序.此 Fuse 驱动程序充当由不同用户级守护程序实现的各种特定文件系统的代理.

除了注册一个新的文件系统之外,FUSE的内核模块还注册一个`/dev/fuse`块设备.该设备充当用户空间 FUSE 守护进程和内核之间的接口.一般来说,守护进程从 `/dev/fuse` 读取 FUSE 请求,处理它们,然后将回复写回 `/dev/fuse`.

下图展示了 FUSE 的高层架构.当用户应用程序在已安装的 FUSE 文件系统上执行某些操作时(黄色部分, 比如 ls), VFS 会将操作路由到 FUSE 的内核驱动程序(蓝色部分). 由驱动程序分配一个FUSE请求结构并将其放入FUSE队列中. 然后 FUSE 的用户级守护进程(绿色部分)通过读取 `/dev/fuse` 从内核队列中选取请求并处理该请求.

![20240526232247](https://raw.githubusercontent.com/learner-lu/picbed/master/20240526232247.png)

因此 FUSE 主要有如下三部分组件

- **FUSE内核模块(蓝色)**:这是与操作系统内核交互的组件,负责将从VFS发的文件系统请求传递给用户空间进程;及将接收到的用户空间进程处理完的请求返回给VFS.
- **FUSE库libfuse(绿色)**:这是提供给用户空间文件系统的库,它提供了一组API来简化文件系统实现.负责和内核空间通信,接收来自/dev/fuse的请求,并将其转化为一系列的函数调用,将结果写回/dev/fuse;

  提供的函数可以对fuse文件系统进行挂载卸载、从linux内核读取请求以及发送响应到内核.

- **用户空间文件系统(绿色)**: 依赖 libfuse 实现的用户态文件系统, 用于处理逻辑

## FUSE 的请求

当FUSE的内核驱动程序与用户空间守护进程通信时,它形成一个FUSE请求结构.请求根据其传达的操作而具有不同的类型. 下表列出了所有 43 种 FUSE 请求类型,按语义分组. 大多数请求都直接映射到传统的 VFS 操作:

![20240526233248](https://raw.githubusercontent.com/learner-lu/picbed/master/20240526233248.png)

大部分接口和内核 VFS 的 `struct file_operation` 接口含义相似, 值得一提的有如下的一些请求类型

### INIT 请求
在文件系统安装过程中,内核会生成INIT请求.这个请求的目的是让用户空间与内核进行协商,以确定运行的协议版本;相互支持的功能集,例如READDIRPLUS或FLOCK支持;各种参数设置,如FUSE预读大小和时间粒度.

用户可以在此时完成系统的初始化, 比如 superblock, gdt 的预读取等等

### DESTROY 请求
与INIT请求相对应,DESTROY请求在文件系统卸载时由内核发送.守护进程在接收到DESTROY请求时,需要执行必要的清理工作.此后,内核将不再发出任何请求,从/dev/fuse读取的数据将返回0,这将导致守护进程正常退出.

### INTERRUPT 请求
如果内核不再需要任何先前发送的请求(例如,当在READ上阻塞的用户进程终止时),它会发出INTERRUPT请求.每个请求都有一个唯一的序列号,INTERRUPT请求使用该序列号来识别需要中断的请求.序列号由内核分配,并在用户空间回复时用于定位已完成的请求.

### FORGET 和 BATCH FORGET 请求
当从dcache中删除一个inode时,内核将FORGET请求传递给用户空间守护进程,守护进程可以选择释放任何相应的数据结构.BATCH FORGET允许内核通过单个请求忘记多个inode.

### ACCESS 请求
当内核评估用户进程是否有权访问文件时,它会生成ACCESS请求.通过处理此请求,FUSE守护程序可以实现自定义权限逻辑.然而,通常用户使用默认权限选项安装FUSE,该选项允许内核根据文件的标准Unix属性(所有权和权限位)授予或拒绝对文件的访问.在这种情况下,不会生成ACCESS请求.

## FUSE 的队列

当程序触发文件系统接口并生成 FUSE 请求之后会进入 FUSE 的多级队列

![20240526233004](https://raw.githubusercontent.com/learner-lu/picbed/master/20240526233004.png)

上图展示了 FUSE(Filesystem in Userspace)的多级队列组织结构.FUSE 维护了五个不同的队列,每个请求在任何时候只属于一个队列.

1. **中断队列(Interrupts Queue)**:当系统需要中断正在处理的请求时,例如用户进程终止了一个正在等待的读操作,FUSE 会把中断请求(INTERRUPT request)放入此队列.中断请求具有**最高优先级**,会首先被传递到用户空间.

2. **遗忘队列(Forgets Queue)**:此队列用于处理遗忘请求(FORGET request),这些请求由内核发出,指示用户空间的守护进程可以释放与特定 inode 相关的资源. FORGET 请求有助于减少缓存压力.

3. **待处理队列(Pending Queue)**:包含同步请求(如元数据操作),这些请求会直接进入此队列等待处理.当用户空间守护进程从 `/dev/fuse` 读取时,会首先从待处理队列中获取请求.

4. **处理队列(Processing Queue)**:当守护进程开始处理来自待处理队列的请求时,这些请求被移动到处理队列.处理队列中的请求当前正在被守护进程处理.

5. **后台队列(Background Queue)**:用于暂存异步请求,例如,当写回缓存启用时,写操作会首先在页面缓存中累积,然后通过 `bdflush` 线程异步刷新脏页.这些异步写请求随后被放入后台队列,并逐渐移入待处理队列.

队列的组织方式允许 FUSE 根据请求的类型和优先级有效地管理请求处理.例如,中断请求和遗忘请求可以快速处理,以避免阻塞更重要的操作.同时,通过限制待处理队列和处理队列中的异步请求数量,FUSE 可以避免因大量后台请求而延迟重要的同步请求.

### 工作流程

总体而言, FUSE 的完整工作流程如下所示

1. **挂载请求与初始化**:
   - 用户或系统管理员通过命令行工具(如`mount.fuse`或`fusermount`)发出指令,请求挂载一个FUSE文件系统到特定的目录(挂载点).
   - 这个挂载操作触发FUSE内核模块的加载(如果尚未加载),该模块是FUSE实现的核心组件之一,负责在内核空间与用户空间之间建立通信机制.
2. **FUSE内核模块初始化**:
   - FUSE内核模块注册自身为VFS(Virtual File System)中的一个文件系统类型,并创建一个特殊的字符设备文件(如`/dev/fuse`),用于后续的用户空间通信.
   - 内核模块配置好与用户空间守护进程的通信参数,包括挂载选项、权限设置等.
3. **用户空间守护进程启动**:
   - FUSE用户空间守护进程(即开发者实现的文件系统逻辑程序)开始运行,它将通过打开并读写`/dev/fuse`设备文件与内核进行交互.
   - 守护进程通过FUSE库(libfuse)注册一系列回调函数,这些函数对应文件系统操作,如打开文件(`open`)、读取(`read`)、写入(`write`)、列出目录内容(`readdir`)等.
4. **系统调用与请求转发**:
   - 当应用程序执行如`open`, `read`, `write`等系统调用来访问挂载点下的文件时,这些调用首先到达内核的VFS层.
   - VFS识别出这是针对FUSE挂载点的请求,于是将调用转发给FUSE内核模块,而不是直接处理.
5. **消息封装与传递**:
   - FUSE内核模块将这些系统调用转换成FUSE协议的格式,封装成消息,然后通过`/dev/fuse`设备文件发送到用户空间.
   - 这些消息包含了请求的操作类型、参数以及必要的上下文信息.
6. **用户空间处理逻辑**:
   - FUSE守护进程读取消息,并根据消息内容调用对应的回调函数.
   - 回调函数执行实际的文件系统操作逻辑,可能涉及读写真实数据存储、网络通信(如云存储)、加密解密等复杂处理.
7. **响应生成与返回**:
   - 处理完成后,守护进程构建响应消息,包含操作结果(成功或失败状态、返回数据等),并通过`/dev/fuse`回传给内核模块.
   - FUSE内核模块将用户空间的响应转换回内核可理解的格式,并通过VFS最终将结果返回给发起原始系统调用的应用程序.

## FUSE的安装和使用

### 安装

在Linux上,可以通过包管理器安装FUSE.如在Ubuntu上,可以使用以下命令安装:

```bash
sudo apt-get install fuse
```

在macOS上,可以使用Homebrew安装:

```bash
brew install osxfuse
```

### 使用

1. **创建文件系统**:编写文件系统的代码,实现所需的功能.比如如何响应读、写、打开、关闭等操作.
2. **编译文件系统**:编译代码,生成可执行文件.
3. **挂载文件系统**:使用`fusermount`命令挂载文件系统.
4. **访问和使用挂载的文件系统**:一旦挂载成功,就可以像操作任何其他文件系统一样来使用这个挂载点.可以使用文件管理器浏览、编辑文件等.
5. **卸载FUSE文件系统**:使用完毕后,通过命令卸载文件系统:

```bash
fusermount -u /mount/point
```

## 参考

- To FUSE or not to FUSE: performance of user-space file systems [FAST'17](https://www.usenix.org/system/files/conference/fast17/fast17-vangoor.pdf)
- [libfuse](https://github.com/libfuse/libfuse)
- [5分钟搞懂用户空间文件系统FUSE工作原理](https://zhuanlan.zhihu.com/p/106719192)
- [FUSE的使用及示例](https://zhoubofsy.github.io/2017/01/13/linux/filesystem-userspace-usage/)
- [u_fs](https://github.com/Tan-Cc/u_fs)
- [ext4fuse](https://github.com/gerard/ext4fuse)
- [fuse-ext2](https://github.com/alperakcan/fuse-ext2)
- [FAT 和 UNIX 文件系统 (磁盘上的数据结构) [南京大学2023操作系统-P28] (蒋炎岩)](https://www.bilibili.com/video/BV1xN411C74V/)
- [14.ext2文件系统](https://www.bilibili.com/video/BV1V84y1A7or/)
- [UserSpace-Memory-Fuse-System](https://github.com/jinCode-gao/UserSpace-Memory-Fuse-System)
- [sshfs](https://github.com/libfuse/sshfs)
- [http://fuse.sourceforge.net/](http://fuse.sourceforge.net/?spm=5176.28103460.0.0.423c3da2xGXQya)
- [Filesystem in Userspace](https://en.wikipedia.org/wiki/Filesystem_in_Userspace)
- [encfs wiki](https://github.com/vgough/encfs/wiki)
- [fuse简介](https://blog.csdn.net/m0_65931372/article/details/126253082)
- [用户态文件系统fuse学习](https://blog.csdn.net/daocaokafei/article/details/115414557)
