
# 文件系统

在 [存储](./storage.md) 中我们介绍了有关存储设备的发展历史, 从最开始的机械打孔, 到磁, 到闪存..., 虽然文件系统和存储设备息息相关, 但这些则更多偏向硬件差异/内部实现/优化, 并不是大多数软件开发者关注的重点. 我们知道硬件设备会通过驱动程序由操作系统内核进行统一管理, 因此本文讨论的更多是偏向操作系统内部文件系统的软件设计与实现.

![20240505230502](https://raw.githubusercontent.com/learner-lu/picbed/master/20240505230502.png)

## 什么是文件系统?

那么什么是文件呢? 对于 UNIX 来说, **文件就是字节**, 文件的内容是由用户来解释的. 任何数据都可以被视为一种文件, 文件只是数据的一种抽象表示方式

那么为了管理文件(数据)在内存/磁盘上的组织形式, 就诞生了文件系统. 抽象的来说, 文件系统是用来**组织**和**存储**计算机数据的. 它的功能类似于一个 `Map<i, str>`, i 为文件对应的唯一索引编号, str 对应文件内容的字节数据.

但显而易见, 使用唯一标识符作为 Map 的索引 key 并不是用户友好的, 因此通常需要提供一个用户友好的命名服务, 主要工作包括:

- **外部名字**到**内部名字**的地址转换(**名字解析**)
- 建立名字和对象/资源之间的关系, 一对一或者多对一
- 删除绑定
- 列举对象绑定的所有名称(反向映射) ...

一般来说命名服务通常选择由用户友好的**字符串**来作为外部名字使用, 名字空间的名字管理也会遵从特定的方法或结构,比如包含位置信息. 最常见的就是**层次式的命名**, 例如 `/a/x`和 `/b/c/x` 的文件名字都是x,但上下文不同. 名字的每个部分都由不同的上下文解释,相同的名字可以用在不同的上下文中表示不同的对象. **在文件系统中,上下文就是目录**, 如下图所示, 一个经典的层次式命名空间

![20240506214350](https://raw.githubusercontent.com/learner-lu/picbed/master/20240506214350.png)

对于文件来说, 除了最重要的文件内容之外也需要一些元数据作为额外的信息记录, 包括文件权限, 类型, 大小, 所属的用户/组, 时间等.

因此一个 `File` 的抽象如下

```c
struct File {
    mode permissions; // 文件权限
    uint32 type, size, user, group; // 文件类型, 大小, 用户/组
    timestamps atime, mtime, ctime; // 修改时间, 创建时间
    content bytes; // 文件内容
};
```

文件系统最核心最重要的工作就是完成**从文件名到文件的一个映射**: `Map<string, File>`

1. 将文件名映射到索引节点, 我们称为 inode_num, 即 `file_path -> inode_num`;
2. 将 inode 映射到文件, 即 `inode_num -> File`

![20240506221237](https://raw.githubusercontent.com/learner-lu/picbed/master/20240506221237.png)

我们注意到第一个映射的文件名是唯一的, 但是**不同的文件名可以有相同的 inode_num 值**, 比如说 `dirA/dirB/..` 和 `dirA/dirC/..` 都指向了 `dirA`; 

> 除此之外 `dirA` 目录还有其下的 `.` 和来自上级目录的 `dirA` 指向它

![20240506231735](https://raw.githubusercontent.com/learner-lu/picbed/master/20240506231735.png)

> 因此一个普通目录的引用数为 [2](https://unix.stackexchange.com/a/101536); `/` 目录的 `.` 和 `..` 都指向它自己
>
> 在同一个文件系统中, inode_num 是唯一的, 硬链接(hard-link)不能跨文件系统, 软链接(soft-link/symbolic link)可以

在这个系统中 `inode_num` 是一个非常重要的桥梁, 也是文件系统的主键(key). 大多数文件系统将其设计为 `uint_32`, 这也限制了文件系统的最大文件个数. 使用整数类型主要是为了方便利用例如红黑树的特性,可以在 O(logN) 时间内完成插入和搜索.

> 整数类型并不是必须的, 如果文件名的字符串比较特殊, 例如 TFS 中将元数据保存在文件名当中, 可以不需要两次映射; 或者所有的文件名可以组成一个紧密的前缀树, 也可以直接以文件名作为主键. 具体取决于文件系统设计之初的需求和场景

可以用来持久化存储的存储介质有很多, 但操作系统能感知的差异大多被硬件控制器抹去了, 因此下文以当前最常用的**磁盘为存储设备代指**. 磁盘在物理上以扇区(Sector)为概念, 逻辑上以块(Block)为概念; 物理磁盘上2010 年以前扇区大小通常是 512B, 其后新的磁盘通常来说是 4KB, 逻辑块早期是 1KB, 现在多为 4KB

> 时至今日仍然是 4KB, 因为最多 64 位操作系统 4 四级页表内存页面大小就是 4KB, Block size 再大反而不好

因此我们可以为文件系统和磁盘做出如下的抽象表示, `FileSystem` 封装统一的外部接口供用户调用, `BlockDevice` 对应逻辑上磁盘, 采用线性的地址, 通过接口和 `FileSystem` 完成交互; `BlockDevice` 又最终和物理磁盘 `Disk` 完成交互, 由磁盘硬件控制器完成从逻辑地址到物理地址(LBA)的转换, 将数据读写到磁盘扇区当中.

```c
// Physical: Logical block addressing (LBA)
struct Disk {
    int readSector(long n, void *buf);
    int writeSector(long n, const void *buf);
}

// Logical: Linear space of contiguous blocks
struct BlockDevice {
    int readBlock(long n, void *buf);
    int writeBlock(long n, const void *buf);
};

struct FileSystem {
    File *open(char *filename, int mode);
    int close(File *file);

    int read(File *, void *buf, int len);
    int write(File *, const void *buf, int len);
    int lseek(File *, int offset, int whence);

    int unlink(char *filename);
    int mkdir(char *path);
};
```

## 数据块(block)

前文我们提到了 Block 这个词, 那么首先需要明晰一点, **扇区**(sector)和**数据块**(block)是两个不同的概念.

- 扇区: **磁盘或光盘上磁道的细分**, 是硬盘的最小单位.对于大多数磁盘,每个扇区存储固定数量的用户可访问数据,传统上硬盘驱动器 (HDD) 为 512 字节,CD-ROM 和 DVD-ROM 为 2048 字节.较新的 HDD 和 SSD 使用 4096 字节 (4 KiB) 扇区,称为高级格式 ([AF](https://en.wikipedia.org/wiki/Advanced_Format))

  第一代高级格式(4K 扇区技术)通过将存储在 8 个 512 字节扇区中的数据合并为一个长度为 4096 字节 (4 KB) 的扇区,更有效地使用存储表面介质.保留了传统 512 字节扇区架构的关键设计元素,特别是扇区开头的标识和同步标记以及扇区末尾的纠错编码 (ECC) 区域.在扇区标头和 ECC 区域之间,合并了 8 个 512 字节扇区,无需在每个单独的 512 字节数据块之间使用冗余标头区域, **节约了空间资源**

  ![20240507233327](https://raw.githubusercontent.com/learner-lu/picbed/master/20240507233327.png)

  > 简而言之, 可以认为现代磁盘(绝大部分)已经是 4 KB 的扇区大小了,  磁盘厂商省材料了, 进而省钱了. ~~陈旧的知识该更新了~~

- 块: 也称为逻辑块,是**文件系统层面的概念**.文件系统不是按照扇区的来读数据, 而是按照是数据块来读取数据,就是说块(block)是文件系统存取数据的最小单位,一般大小也是4KB(这个值可以修改,在格式化分区的时候修改)

  BSD FFS 的作者也曾在 [一篇杂志](https://www.usenix.org/system/files/login/articles/584-mckusick.pdf) 中提及, "增加 block size 可以使文件系统的性能提升"

  ![20240505233657](https://raw.githubusercontent.com/learner-lu/picbed/master/20240505233657.png)

  在其[发展历史](https://freebsdfoundation.org/wp-content/uploads/2016/04/A-Brief-History-of-the-BSD-Fast-Filesystem.pdf)中也可以看到 block size 的不断变化

大多数文件系统都基于块设备,这是负责存储和检索指定数据块的硬件的抽象级别. 尽管文件系统中的块大小(通常)是物理块大小的倍数, 但是由于文件长度通常不是块大小的整数倍,因此文件的最后一个块可能部分为空而产生内部碎片.

> 一些较新的文件系统,如 [Btrfs](https://en.wikipedia.org/wiki/Btrfs) 和 FreeBSD UFS2,试图通过称为[块子分配和尾部合并](https://en.wikipedia.org/wiki/Block_suballocation)的技术来解决这个问题.其他文件系统(如 [ZFS](https://en.wikipedia.org/wiki/ZFS))支持可变块大小

因此一个合理的要求是 disk sector size <= block size <= memory page size (4KB). block size 最好是 sector size 的整数倍, 这样就可以分写到几个扇区; 而 block size 的大小也不能超过 page size, 否则一个虚拟页面也无法存储. 现在都是 4KB, 算是巧合也是必然

## 设计假设

在计算机系统领域, 任何时候如果想要设计一个 system 去解决一个实际的问题, 比如说文件系统, 这是一个很实际的问题, 因此我们**不能抛开 workload 来谈系统**. 因此当我们设计文件系统的时候就需要先去考虑一个对应的使用场景. 除了针对某些特定场景/服务优化的情况, 文件系统应当在大部分实际的 workload 情况下表现良好

因此我们需要在设计之初引入一些基本的假设

|summary|findings|
|:--:|:--:|
|大多数文件都很小|人类很难维护超大的文本类型文件, 2K 是比较适中的大小|
|平均文件大小在增长|大约200KB是中位数|
|大多数数据保存在大文件当中|很少的大文件占据了大多数的空间, 比如说一部 GB 级的电影|
|文件系统会保存很多数量的文件|大概100K是中位数|
|文件系统大概会有一半是满的|虽然磁盘大小在增长, 大部分文件系统的空间会占满一半|
|目录通常不会很大|很少有目录有大量的文件, 大约20个或更少|

也有早期的 [论文](https://dl.acm.org/doi/abs/10.1145/1113361.1113364) 研究过系统中文件大小的变化规律, 整体来说文件的体积是在不断变大的, web 服务器上的小文件是很多的(很多小脚本, 小图标)

![20240507233556](https://raw.githubusercontent.com/learner-lu/picbed/master/20240507233556.png)

在文件系统设计时主要考虑两个方面, **如何管理和组织数据和元数据** 以及 **如何在该结构上实现访问(open/read/write)**; 例如采用何种方式组织? 数据还是树? 读或者写入新数据的时候, 如何更新数据结构, 如何分配和回收块? 等等

当然也可能根据使用场景来调整块大小, 大块可以显著提高吞吐量, 而且元数据的占比更小; 小块可以有更高效的空间占用, 对于很多小文件来说比较合适.

操作系统中的文件系统管理着众多的文件,每个文件都是一个独立的字节序列,即一个虚拟磁盘.用户和应用程序可以通过文件系统对这些文件进行增删改查等操作.为了便于管理和访问,需要文件系统提供**命名管理**.每个文件都有一个唯一的名称,如"a.txt"或"bin/ls",用户可以通过这些名称来访问和管理文件, 还要给他提供文件的检索/遍历的这些功能; 文件系统还需要提供**数据管理**, 以便随机读写和调整文件大小

## 实现

假设我们有一块磁盘, 第一件事就是将其按照 block(4KB) 划分. 假设我们的磁盘容量很小, 一共可以划分 0-63 个块, 如下所示

![20240509104541](https://raw.githubusercontent.com/learner-lu/picbed/master/20240509104541.png)

那在这些blocks中应该存储些什么?文件系统的基础要素自然是文件,而文件作为一个数据容器的逻辑概念,本质上是一些字节构成的集合,这些字节就是文件的user data(对应下图的"D")

![20240509104851](https://raw.githubusercontent.com/learner-lu/picbed/master/20240509104851.png)

> 图中的 D 目前不指代任何实际的数据, 只是暂时划分出来一块数据区域; 将后 56 个块(8-63)标记位数据区域也只是简化考虑

除了文件本身包含的数据,还有文件的访问权限、大小和创建时间等控制信息,这些信息被称为**元数据(meta data)**. 这些meta data存储的数据结构就是inode(对应下图的"I"), 这个 inodes 区域也称为 **inode table**

在早期的Unix系统中,这些nodes是通过数组组织起来的,因此需要依靠index来索引数组中的node.假设一个inode占据256字节,那么一个4KB的block可以存放16个inodes,使用5个blocks可以存放80个inodes,也就是最多支持80个文件

> 我们的磁盘一共只有 64 个块, 由于每一个文件至少存一个块(也可能占据好几个), 所以文件数量的上限也就是 64. 因为每一个文件都对应一个 inode, 一个块中可以保存 16 个 inode, 所以最佳的分配方式其实是只使用 4 个 block 用于 inode table(4x16=64), ~~分给了5个只是看着连续~~

![20240509105539](https://raw.githubusercontent.com/learner-lu/picbed/master/20240509105539.png)

> 在 [inode](./inode.md) 中将详细介绍该结构

此时我们已经有了保存文件数据的区域 D 和保存对应文件元数据的区域 I 了, 但是同内存分配一样,当有了新的数据产生时,我们需要选择一个空闲的block来存放数据,此外还需要一个结构(allocate structure)来**追踪这些inodes和data blocks的分配和释放情况**,以判断哪些是已用的,哪些是空闲的

最简单的办法就是使用bitmap,包括记录inode使用情况的bitmap(对应下图的"i"),和记录data block使用情况的bitmap(对应下图的"d").空闲就标记为0,正在使用就标记为1

![20240509110534](https://raw.githubusercontent.com/learner-lu/picbed/master/20240509110534.png)

> 现代 CPU 可以通过多种CPU指令集来加速 bitmap 的查找, 这些指令集通常属于单指令多数据(SIMD)技术,它们可以同时对多个数据执行相同的操作,从而提高处理速度, 例如 MMX, SSE, AVX
>
> 找到磁盘剩余空间的大小很快, 因为它保存在 inode bitmap 中, 可以在 O(1) 的时间内完成

可能有人注意到了这里使用了两个blocks来分别存储inode bitmap和data block bitmap,每个block可以容纳的bits数量是 4K * 8 = 32K, 这远远超出了我们实际使用的80个inodes和56个data blocks, ~~这也只是为了简化~~

还剩下开头的一个block,这个block是留给superblock的(对应下图的"S")

![20240509111401](https://raw.githubusercontent.com/learner-lu/picbed/master/20240509111401.png)

superblock包含了一个文件系统所有的控制信息,比如文件系统中有多少个inodes(80)和data blocks(56),inode的信息起始于哪个block(这里是第3个),可能还有一个区别不同文件系统类型的magic number,版本信息等等.因此,superblock可理解为是文件系统的meta data.

![20240509112819](https://raw.githubusercontent.com/learner-lu/picbed/master/20240509112819.png)

- **Magic**: 用来告诉操作系统该磁盘是否已经拥有了一个有效的文件系统.
- **Blocks**: blocks 总数(64)
- **InodeBlocks**: 其中属于 inode 的 block 数(5)
- **Inodes**: 在 InodeBlocks 中存在多少个 inodes(16)

> 图中没有标出 InodeIndex(3)
>
> 这里的 superblock 的概念**并不准确**, 准确来说合并了 superblock 和 Block Group Descriptor 两个概念, 我们将在 [disk-layout](./disk-layout.md) 中详细介绍

在挂载文件系统时,操作系统会首先读取超级块,初始化各种参数,然后将卷附加到文件系统树上.这样当访问卷中的文件时,系统就能准确地知道在哪里可以找到所需的磁盘结构了.

此时如果想要读出 inode_num 为 32 的文件, 首先通过 superblock 得到 inode_table 的块号(3), 然后计算得到偏移量 4KB * 3 = 12KB, 然后找到 32 号的 inode 的偏移量 32 * sizeof(struct inode) = 8KB, 然后相加读取该地址的 inode 结构体, 然后读取其中的 blocks 字段得到该文件包含的数据块号, 然后再通过偏移量读取出对应的数据块即可, 如下所示

![20240509214353](https://raw.githubusercontent.com/learner-lu/picbed/master/20240509214353.png)

> 如果是 34 号的话还需要再加上block内的两个 inode 的偏移量 (+2*256)

## 问题: 读写放大

相比于内存 RAM 的读写设计, 磁盘没有办法做到 1 bit 的修改. 其中一个主要的挑战是**读写放大**.例如,当用户程序仅需要修改一个文件中的一小部分数据时, 文件系统仍然需要将一整个数据块读出, 修改后再将其写入.

然而,我们也有一些有利因素.如果我们从整个操作系统的角度来考虑,而不是仅仅关注单个应用程序的单次读写操作,我们会发现**许多程序会持续地访问大量文件**.例如,浏览器在启动时会读取多个文件,而视频编辑软件在运行时也会加载许多动态链接库和数据.通过合理地组织这些数据,我们可以利用数据**局部性**原理,优化文件的存储和访问模式.这意味着,如果我们将经常一起访问的数据放置在相邻的位置,那么当一个数据被读取时,其相邻的数据也可能很快被再次访问,从而减少读写放大的影响.

此外,缓存技术是缓解读写放大的有效手段.文件系统可以**将频繁访问的数据保存在内存缓存中**,这样当再次访问这些数据时,就可以直接从内存中读取,而不是从磁盘.对于写操作,文件系统可以将数据先写入内存缓存,然后在合适的时机批量写入磁盘.这种方法可以将**多次磁盘写入合并为一次**,减少了对磁盘的I/O操作,从而有效缓解读写放大问题.

这些策略在计算机系统的基础课程中已有介绍,它们是提高计算机性能的重要技术.通过这些方法,文件系统可以更高效地管理数据,提供更好的用户体验.因此,在设计文件系统时,我们需要综合考虑这些因素,以实现高效、稳定的数据存储和访问.

在Linux系统中,磁盘设备不仅仅是一个简单的字符设备,它的操作远比基本的读写更加复杂. 磁盘是所有程序共享的, 如果单纯的将这一大片地址空间丢给应用程序, 那么它们之间的数据访问和管理将是噩梦. 因此需要以一种有序的方式访问磁盘上的数据.如果仅仅由设备驱动程序将磁盘抽象为一个可读可写的字节序列,这种抽象是不够的.因为**这样的抽象没有考虑到磁盘的共享特性和访问模式**,可能会导致应用程序之间的数据访问冲突和效率低下.

## 虚拟文件系统(VFS)

Linux操作系统中实现了一个称为**block IO层**的结构,这一层在多CPU环境下工作,负责处理来自上层应用程序和文件系统的磁盘访问请求.在这个层面上,我们可以将其理解为一个中间层,**它接收来自上层的请求并将其转换为对磁盘的块级操作**.

![20240408160414](https://raw.githubusercontent.com/learner-lu/picbed/master/20240408160414.png)

> 关于这一部分的详细讨论见 [vfs](./vfs.md)

当你想要向磁盘写入数据时,例如写入一系列的数据块(1, 2, 3, ..., 9),你的请求首先会被提交给block IO层.在这一层中,存在一个队列,它负责管理和调度所有的读写请求.block IO层的API提供了基本的读块和写块操作,同时也支持确保所有写入操作都已持久化到存储设备的功能,这通常是通过fsync系统调用实现的.

> ```bash
> man 2 fsync
> ```
>
> fsync是一个同步操作,当你认为某些数据非常重要,需要立即持久化到存储设备时,你可以使用fsync来**确保所有待写入的数据都被安全地存储**.例如,在拔出U盘之前,你可以使用fsync来确保所有缓存中的数据都被写入到U盘中.

在Linux中,除了IO调度的复杂性,对于上层应用程序和文件系统来说,它们不需要关心底层的实现细节.文件系统层和虚拟文件系统(VFS)将不同类型的请求(如读取数据块、写入数据块等)封装并提交给block IO层的队列.最终,这些请求会通过总线和磁盘接口传递给磁盘驱动程序,由驱动程序完成实际的磁盘读取和写入操作.

## 发展历史

早期的 Unix 文件系统简称为 FS.FS 仅包括引导块、超级块、一团 inode 和数据块.这适用于早期Unix设计的小磁盘,但随着技术的进步和磁盘的变大,在索引节点群和它们所引用的数据块之间来回移动磁头会导致**抖动**.

马歇尔·柯克·麦库西克(Marshall Kirk McKusick)当时是伯克利的研究生,他优化了 UNIX V7 FS 布局,通过发明柱面组来创建 BSD 4.2 的 Fast File System(**FFS**),柱面组将磁盘分解成更小的块,每个组都有自己的 inode 和数据块.

Linux 内核的早期开发是在 MINIX 操作系统下进行的交叉开发. 作为 Linux 的第一个文件系统, **Minix** 文件系统基本上没有错误,但在内部使用 16 位偏移量,因此最大大小限制仅为 64 MB,文件名长度限制为 14 个字符. 由于这些限制, Linux 开始着手开发可以替代的本机文件系统.

为了简化新文件系统的添加并提供通用文件 API, [VFS](./vfs.md) 被添加到 Linux 内核中.扩展文件系统 (**ext**) 于 1992 年 4 月发布,是第一个使用 VFS API 的文件系统,并包含在 Linux 版本 0.96c 中. ext 文件系统解决了 Minix 文件系统中的两个主要问题(最大分区大小和文件名长度限制为 14 个字符),并允许 2 GB 的数据和最多 255 个字符的文件名.

但 ext 保留了一些问题,例如性能不佳和缺少一些日期戳.取代 ext 的两个竞争者很快被开发出来:**ext2** 和 Xiafs. 它们有着相同的目标:提供良好的性能、合理的限制并修复 ext.最初,Xiafs 比 ext2 更稳定,但作为**对 MINIX 文件系统的相当简约的修改,它不太适合未来的扩展**. 最终的结果是,Xiafs 变化很小,而 ext2 有了很大的发展,迅速提高了稳定性和性能,并增加了扩展.ext2 经过一段时间的调整,迅速成为 Linux 的标准文件系统.从那时起,ext2 已经发展成为一个非常成熟和健壮的文件系统.

ext2 是对扩展文件系统的大修, 融合了 Berkeley Fast File System(FFS) 的许多想法. 此外 ext2 在设计时还考虑了可扩展性,在其许多磁盘数据结构中留有空间供将来的版本使用. 从那时起,ext2 一直是 VFS API 许多新扩展的测试平台.

由于ext2文件系统驱动程序不支持 2038 年之后的日期(32位整数溢出带来的[Y2038问题](https://en.wikipedia.org/wiki/Year_2038_problem)), 因此如今 linux 6.9 中已经于该 [commit](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=b960e8093e7a57de98724931d17b2fa86ff1105f) 正式 [弃用 ext2](https://www.phoronix.com/news/Linux-6.9-Deprecates-EXT2), 取而代之的是可以兼容 ext2 的 ext4

ext2 的继任者 **ext3** 引入了[日志](https://en.wikipedia.org/wiki/Journaling_file_system)的概念, 以提高系统突然停止时文件系统的可靠性. 虽然 ext3 的性能(速度)不如竞争对手的 Linux 文件系统(如 ext4、JFS、ReiserFS 和 XFS),但 ext3 有一个显着的优势,因为**它允许从 ext2 进行就地升级,而无需备份和恢复数据**. 由于其相对简单和更广泛的测试基础,它也被认为比其他 Linux 文件系统更安全

ext3 缺少"现代"文件系统功能,例如动态 inode 分配和扩展.这种情况有时可能是一个缺点,但对于**可恢复性**来说,这是一个显着的优势.文件系统元数据全部位于固定的已知位置,并且数据结构具有一定的冗余性.在重大数据损坏中,ext2 或 ext3 可能是可恢复的,而基于树的文件系统可能不可恢复

ext3 的继任者 **ext4** 最初是 ext3 的一系列向后兼容扩展, 其他 Linux 内核开发者出于稳定性的考虑,反对接受 ext3 的扩展, 并提议 fork ext3 的源代码,将其重命名为 ext4,并在那里执行所有开发,而不会影响现有的 ext3 用户. Ext4 在性能、可扩展性和可靠性方面引入了许多新的改进.最值得注意的是,ext4 支持大小为 1 EB (1024 x 1024 TB)的文件系统

---

文件系统可以分为:

- **磁盘文件系统**, 利用磁盘存储介质在短时间内随机寻址数据的能力.其他考虑因素包括访问最初请求的数据的速度,以及可能还会请求以下数据的预期.这允许多个用户(或进程)访问磁盘上的各种数据,而不考虑数据的顺序位置
  例如 FAT(FAT12、FAT16、FAT32)、exFAT、NTFS、ReFS、HFS 和 HFS+、HPFS、APFS、UFS、ext2、ext3、ext4、XFS、btrfs, 绝大部分文件系统均是为磁盘设计考虑的
- **闪存文件系统**, 考虑了闪存设备的特殊能力、性能和限制.通常,磁盘文件系统可以使用闪存设备作为底层存储介质, 上述的大部分磁盘文件系统都可以在闪存文件系统上表现出色, 但也有单独为闪存介质特性设计的文件系统,例如 UBIFS, YAFFS, SPIFFS
- **磁带文件系统**, 磁带是顺序存储介质,其随机数据访问时间比磁盘长得多,这给通用文件系统的创建和有效管理带来了挑战, 例如 IBM 的 [Linear Tape File System](https://en.wikipedia.org/wiki/Linear_Tape_File_System)
- **数据库文件系统**, 与分层结构化管理不同,或者除了分层结构化管理之外,文件还按其特征(如文件类型、主题、作者或类似的丰富元数据)进行标识. 其他还有例如非常大的文件系统,由[Apache Hadoop](https://en.wikipedia.org/wiki/Apache_Hadoop)和[Google File System(GFS)](https://en.wikipedia.org/wiki/Google_File_System)等应用程序所体现,使用一些数据库文件系统概念
- **事务性文件系统**, 某些程序需要进行多个文件系统更改,或者如果一个或多个更改由于任何原因失败,则不进行任何更改.
  例如,正在安装或更新软件的程序可能会写入可执行文件、库和/或配置文件.如果某些写入失败并且软件被部分安装或更新,则软件可能已损坏或无法使用.密钥系统实用程序(如命令行界面)的更新不完整可能会使整个系统处于不可用状态.

  事务处理引入了原子性保证,确保事务内部的操作要么全部提交,要么事务可以中止,系统丢弃其所有部分结果.这意味着如果发生崩溃或电源故障,恢复后,存储状态将是一致的.软件将完全安装,或者失败的安装将完全回滚,但系统上不会留下不可用的部分安装.事务还提供隔离保证,这意味着在事务提交之前,事务中的操作对系统上的其他线程是隐藏的,并且系统上的干扰操作将与事务一起正确序列化.

- **网络文件系统**, 它充当远程文件访问协议的客户端,提供对服务器上文件的访问.使用本地接口的程序可以透明地创建、管理和访问远程联网计算机中的分层目录和文件.网络文件系统的示例包括 NFS 、AFS、SMB 协议的客户端,以及用于 FTP, SSH (sshfs)
- **用户态文件系统**, 例如 FUSE
- **分布式文件系统**

## FHS

macOS 是 UNIX 的内核 (BSD), 但不遵循 Linux FHS

![20240408165721](https://raw.githubusercontent.com/learner-lu/picbed/master/20240408165721.png)

## 参考

- [设备驱动程序与文件系统 (Linux 设备驱动;目录管理 API) [南京大学2023操作系统-P27] (蒋炎岩)](https://www.bilibili.com/video/BV1m24y1A7Fi/)
- [FAT 和 UNIX 文件系统 (磁盘上的数据结构) [南京大学2023操作系统-P28] (蒋炎岩)](https://www.bilibili.com/video/BV1xN411C74V)
- [【存储知识】文件系统与硬盘存储(分区、格式化、挂载、inode、软链接与硬链接)](https://zhuanlan.zhihu.com/p/693575325)
- [Disk sector](https://en.wikipedia.org/wiki/Disk_sector)
- [Linux 文件系统(一):抽象](https://www.bilibili.com/video/BV1jM411W7jV/)
- [Linux 文件系统(二):磁盘](https://www.bilibili.com/video/BV11L411y785/)
- [Linux 文件系统(三):分块读写](https://www.bilibili.com/video/BV1QT411r738/)
- bsd ffs
  - [FreeBSDFridays: History of the BSD Fast Filesystem](https://www.youtube.com/watch?v=r5nFapl5C1s)
  - [A Brief History of the BSD Fast Filesystem](https://www.youtube.com/watch?v=Lqctf-tfBX8)
- ext
  - [Ext2](https://en.wikipedia.org/wiki/Ext2)
  - [Unix File System](https://en.wikipedia.org/wiki/Unix_File_System)
  - [Ext3](https://en.wikipedia.org/wiki/Ext3)
- [OSTEP fs implementation](https://pages.cs.wisc.edu/~remzi/OSTEP/file-implementation.pdf)
- [文件系统的原理](https://zhuanlan.zhihu.com/p/106459445)

FHS

- [Filesystem Hierarchy Standard](https://en.wikipedia.org/wiki/Filesystem_Hierarchy_Standard)
- [FHS 3.0 文档](https://refspecs.linuxfoundation.org/FHS_3.0/fhs-3.0.pdf)
- [【linux】rootfs根文件系统镜像制作](https://blog.csdn.net/iriczhao/article/details/127078414)
- [直接引导Linux内核.md](https://www.bookstack.cn/read/learn-kvm/docs-QEMU%E5%8A%9F%E8%83%BD-%E7%9B%B4%E6%8E%A5%E5%BC%95%E5%AF%BCLinux%E5%86%85%E6%A0%B8.md)
- [深入理解 Linux 启动过程 | QEMU 启动 linux 内核和自制根文件系统](https://cloud.tencent.com/developer/article/2347447)
- [cyberciti description-of-linux-file-system-directories](https://www.cyberciti.biz/tips/description-of-linux-file-system-directories.html)
- [The Filesystem Hierarchy Standard of Linux](https://zhuanlan.zhihu.com/p/23862856)
