
# disk-layout

在 [filesystem](./filesystem.md) 的实现中我们介绍了文件系统按照如下形式对磁盘进行组织

![20240509111401](https://raw.githubusercontent.com/learner-lu/picbed/master/20240509111401.png)

同时为了追踪这些inodes(I)和data blocks(D)的分配和释放情况, 文件系统使用bitmap来分别记录 inode 和 data blocks 的使用情况, 即 inode bitmap(i) 和 data block bitmap(d)

由于 d 只占了一个数据块(4KB), 所以上述结构所能表示的数据块总量为 4K x 8 = 32K, 总大小为 32K x 4KB = 128MB. 由 inode ratio 16K 可得共需要 8K 个 inode, 计算可得 inode table 需要 8K * 256B/4KB = 512 个 blocks, 除去 superblock, GDT, bitmap 剩余的保存数据块, 结构如下所示

![20240516100550](https://raw.githubusercontent.com/learner-lu/picbed/master/20240516100550.png)

> 其中 inodes(inode table) 的数量占比由 inode ratio 决定, 在 [inode](./inode.md) 中介绍过了

传统UNIX文件系统采用的ext文件系统引入了块组(block group)概念,以增强数据的局部性,提高硬盘驱动器(HDD)的文件读写吞吐量,减少寻道时间和距离. 但是对于SSD或闪存等非机械存储介质并不存在这种物理结构, 因此这种设计在SSD上意义不大

## GDT

GDT 是块组描述符表的缩写, 前文我们认为所有和块组相关的信息都保存在 superblock 当中, 但是实际上在 ext4 文件系统中,`group_desc`(组描述符)和 `superblock`(超级块)都是元数据的一部分,但它们各自扮演着不同的角色:

1. **超级块(Superblock)**:
   - 描述整个文件系统的状态,包括文件系统的总大小、块组数量、块和索引节点的大小、文件系统的挂载次数、最后一次挂载的时间等.
   - 包含文件系统的全局信息,如文件系统的特征、兼容性标志、错误处理行为等.
   - 为了文件系统的可靠性,超级块在文件系统的多个位置有备份,以防某个部分损坏.
   - 用于文件系统检查和挂载过程中,确定文件系统是否可以安全地被访问.

   > [ext4_super_block](https://github.com/luzhixing12345/klinux/blob/63286f3344a3a2a227648da72f5c7fccf5c1f428/fs/ext4/ext4.h#L1289-L1418)

2. **组描述符(Group Descriptor 或 Block Group Descriptor)**:
   - 描述文件系统中每个块组的元数据,包括块位图、索引节点位图、索引节点表的位置,以及该组内空闲块和索引节点的数量等.
   - 每个块组有一个对应的组描述符,包含了该组特有的信息.
   - 用于快速定位某个特定块组的资源使用情况,如查找空闲块或索引节点.
   - 组描述符通常存储在文件系统的块组0中,并且每个组描述符的大小是固定的.

   > [ext4_group_desc](https://github.com/luzhixing12345/klinux/blob/63286f3344a3a2a227648da72f5c7fccf5c1f428/fs/ext4/ext4.h#L390-L415)

简而言之,超级块提供了**整个文件系统的概览信息**,而组描述符则提供了**每个块组的详细信息**.两者共同协助文件系统管理数据和维护其完整性.

超级块(super block)和块组描述符(group descriptor)是文件系统的关键元数据,它们不仅在文件系统级别上存在主备份,还会在其他块组中多次备份,以确保在主备份损坏时,仍能通过其他备份恢复文件系统,避免数据丢失和系统尺寸、分布信息的不可恢复性.

## ext2 块组结构

传统的 ext2 块组结构和上图相同, 我们可以创建一个 1000MB 的文件, 然后将其格式化为 ext2 格式

```bash
dd if=/dev/zero of=test.img bs=1M count=1000
mkfs.ext2 test.img
```

可以使用 dumpe2fs 来查看 ext 文件系统的一些信息

```bash
dumpe2fs test.img
```

由于输出内容较多, 这里列出相对重要的信息. 文件大小为 1000MB, 所以对于 4KB 的 block

- `Block count` 为 256000
- `Inode count` 为 64000.
- 由于每个 block group 为 128MB, 所以分为 8 组, `Inodes per group` 为 8000
- inode 大小为 256(每个block可以存16个inode), 因此 `Inode blocks per group` 为 500, 即每个组的 inode table 长度为 500
- 其余空间保存 data block
- 1/3/5/7 group 额外保存superblock的备份(Backup superblock), 其余 group 不需要保存, data block 的剩余空间会大一些

> 这里省略了对于 `Reserved GDT blocks` 的描述, 它的设计是为了后续的扩展, ~~不过由于 EXT2 已经被遗弃了所以不要在意~~

![20240516113949](https://raw.githubusercontent.com/learner-lu/picbed/master/20240516113949.png)

## ext4 flex group

上图中 ext2 group 看似不错但其实有一点小问题, 每个 block group 总共 32768 个块, 但是由于需要排除 superblock, GDT, bitmap, inode table 等实际剩余的 data block 的数量并不到 32K 了, 而且数据块之间也并不连续, 由 bitmap/inode table 间隔打断, 也不利于数据的连续存储

因此 ext4 采用 flex group 的存储方案, 如下图所示

![20240516114033](https://raw.githubusercontent.com/learner-lu/picbed/master/20240516114033.png)

> 这里 group2 中的 data block 并不是从 65535 开始的, 跳过的 4096 个 block 是 ext4 的日志(Journaling)

注意这里的 bitmap 和 inode table 的 block id 是连续的, 也就是说 8 个 block group 并不是分别保存的, 而是**统一保存** 8 个块的 bitmap 和 inode table. 除此之外没有 superblock 备份的 group 之间可以**连续保存 data block**, 例如 group5 和 group6 的区域的序号是连续的, 可以有更大的连续空间保存文件

这样的数据布局一方面使得 metadata 的存储集中, 有更好的**布局性**; 同时将所有 group 的元数据都保存到第一个 group 中, 那么其余的 group 可以省出 bitmap 和 inode table 的空间来保存更多的数据, 且 data block 可以获得更大的连续空间

## 参考

- [ext4 disk layout](https://ext4.wiki.kernel.org/index.php/Ext4_Disk_Layout)
- [Linux 文件系统(七):磁盘布局](https://www.bilibili.com/video/BV1JT41167WS/)
- [metebalci a-minimum-complete-tutorial-of-linux-ext4-file-system](https://metebalci.com/blog/a-minimum-complete-tutorial-of-linux-ext4-file-system/)