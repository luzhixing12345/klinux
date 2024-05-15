
# disk-layout

在 [filesystem](./filesystem.md) 的实现中我们介绍了文件系统按照如下形式对磁盘进行组织

![20240509111401](https://raw.githubusercontent.com/learner-lu/picbed/master/20240509111401.png)

> 其中 inodes(inode table) 的数量占比由 inode ratio 决定, 在 [inode](./inode.md) 中介绍过了

同时为了追踪这些inodes(I)和data blocks(D)的分配和释放情况, 文件系统使用bitmap来分别记录 inode 和 data blocks 的使用情况, 即 inode bitmap(i) 和 data block bitmap(d)

由于 d 只占了一个数据块(4KB), 所以上述结构所能表示的最大容量为 4K x 8 x 4KB = 128MB. 我们将上述的完整结果文件系统引入了块组(block group)概念,以增强数据的**局部性**

在 ext4 文件系统中,`group_desc`(组描述符)和 `superblock`(超级块)都是元数据的一部分,但它们各自扮演着不同的角色:

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
简而言之,超级块提供了整个文件系统的概览信息,而组描述符则提供了每个块组的详细信息.两者共同协助文件系统管理数据和维护其完整性.

## ext 文件系统参数

||ext2|ext4|
|:--:|:--:|:--:|
|inode size(bytes)|128|256|
|logical block num(bit)|32|32|
|physical block num(bit)|32|48|
|max fs size|4GB|1EB|
|max file size|16GB|16TB|

> - ext2 max fs size = 1KB * 2^32 = 4GB
> - ext2 max file size = 1KB * (12 + 256 + 256x256 + 256x256x256) ~= 16GB
> - ext4 max fs size = 4KB * 2^48 = 1EB
> - ext4 max file size = 4KB * 2^32 = 16TB

如果块大小为 4KB, 那么对于一个 32 位的 block number (ext2)来说, 可以计算得到文件系统支持的最大容量为 `4KB * 2^32 = 16TB`, 而对于一个 48 bit 的block number(ext4) 足以支持 1EB(1024PB)

> 但是因为考虑到块组(block group)的上限为 256TB, 128MB pre group, 因此需要开启 meta block group 的选项

用一个 block 来存 bitmap, 对于 4KB 的 block 有 32K 个 bits, 因此每个组最多 32K 个 blocks, 因此每个组最多 32K x 4KB = 128MB

传统UNIX文件系统采用的ext文件系统引入了块组(block group)概念,以增强数据的**局部性**,提高硬盘驱动器(HDD)的文件读写吞吐量,减少寻道时间和距离.个人猜测,对于SSD或闪存等非机械存储介质,块组的概念可能不太重要.此外,超级块(super block)和块组描述符(group descriptor)是文件系统的关键元数据,它们不仅在文件系统级别上存在主备份,还会在其他块组中多次备份,以确保在主备份损坏时,仍能通过其他备份恢复文件系统,避免数据丢失和系统尺寸、分布信息的不可恢复性.

```bash
(base) kamilu@LZX:~/klinux$ dd if=/dev/zero of=test.img bs=1M count=1000
1000+0 records in
1000+0 records out
1048576000 bytes (1.0 GB, 1000 MiB) copied, 0.562181 s, 1.9 GB/s
(base) kamilu@LZX:~/klinux$ mkfs.ext4 test.img
mke2fs 1.46.5 (30-Dec-2021)
Discarding device blocks: done
Creating filesystem with 256000 4k blocks and 64000 inodes
Filesystem UUID: 454271eb-2907-4242-b324-0e3c0d02bbcf
Superblock backups stored on blocks:
        32768, 98304, 163840, 229376

Allocating group tables: done
Writing inode tables: done
Creating journal (4096 blocks): done
Writing superblocks and filesystem accounting information: done
```

```bash
(base) kamilu@LZX:~/klinux$ dumpe2fs test.img
dumpe2fs 1.46.5 (30-Dec-2021)
Filesystem volume name:   <none>
Last mounted on:          <not available>
Filesystem UUID:          454271eb-2907-4242-b324-0e3c0d02bbcf
Filesystem magic number:  0xEF53
Filesystem revision #:    1 (dynamic)
Filesystem features:      has_journal ext_attr resize_inode dir_index filetype extent 64bit flex_bg sparse_super large_file huge_file dir_nlink extra_isize metadata_csum
Filesystem flags:         signed_directory_hash
Default mount options:    user_xattr acl
Filesystem state:         clean
Errors behavior:          Continue
Filesystem OS type:       Linux
Inode count:              64000
Block count:              256000
Reserved block count:     12800
Overhead clusters:        8742
Free blocks:              247252
Free inodes:              63989
First block:              0
Block size:               4096
Fragment size:            4096
Group descriptor size:    64
Reserved GDT blocks:      124
Blocks per group:         32768
Fragments per group:      32768
Inodes per group:         8000
Inode blocks per group:   500
Flex block group size:    16
Filesystem created:       Wed May 15 19:16:48 2024
Last mount time:          n/a
Last write time:          Wed May 15 19:16:48 2024
Mount count:              0
Maximum mount count:      -1
Last checked:             Wed May 15 19:16:48 2024
Check interval:           0 (<none>)
Lifetime writes:          521 kB
Reserved blocks uid:      0 (user root)
Reserved blocks gid:      0 (group root)
First inode:              11
Inode size:               256
Required extra isize:     32
Desired extra isize:      32
Journal inode:            8
Default directory hash:   half_md4
Directory Hash Seed:      b0c51098-0ca4-4c83-8284-07a9de170c16
Journal backup:           inode blocks
Checksum type:            crc32c
Checksum:                 0x1739a56b
Journal features:         (none)
Total journal size:       16M
Total journal blocks:     4096
Max transaction length:   4096
Fast commit length:       0
Journal sequence:         0x00000001
Journal start:            0


Group 0: (Blocks 0-32767) csum 0xf318 [ITABLE_ZEROED]
  Primary superblock at 0, Group descriptors at 1-1
  Reserved GDT blocks at 2-125
  Block bitmap at 126 (+126), csum 0x21c79edf
  Inode bitmap at 134 (+134), csum 0xc5748499
  Inode table at 142-641 (+142)
  28620 free blocks, 7989 free inodes, 2 directories, 7989 unused inodes
  Free blocks: 4148-32767
  Free inodes: 12-8000
Group 1: (Blocks 32768-65535) csum 0x8925 [INODE_UNINIT, BLOCK_UNINIT, ITABLE_ZEROED]
  Backup superblock at 32768, Group descriptors at 32769-32769
  Reserved GDT blocks at 32770-32893
  Block bitmap at 127 (bg #0 + 127), csum 0x00000000
  Inode bitmap at 135 (bg #0 + 135), csum 0x00000000
  Inode table at 642-1141 (bg #0 + 642)
  32642 free blocks, 8000 free inodes, 0 directories, 8000 unused inodes
  Free blocks: 32894-65535
  Free inodes: 8001-16000
Group 2: (Blocks 65536-98303) csum 0x7bf0 [INODE_UNINIT, ITABLE_ZEROED]
  Block bitmap at 128 (bg #0 + 128), csum 0x48b72194
  Inode bitmap at 136 (bg #0 + 136), csum 0x00000000
  Inode table at 1142-1641 (bg #0 + 1142)
  28672 free blocks, 8000 free inodes, 0 directories, 8000 unused inodes
  Free blocks: 69632-98303
  Free inodes: 16001-24000
Group 3: (Blocks 98304-131071) csum 0xe45e [INODE_UNINIT, BLOCK_UNINIT, ITABLE_ZEROED]
  Backup superblock at 98304, Group descriptors at 98305-98305
  Reserved GDT blocks at 98306-98429
  Block bitmap at 129 (bg #0 + 129), csum 0x00000000
  Inode bitmap at 137 (bg #0 + 137), csum 0x00000000
  Inode table at 1642-2141 (bg #0 + 1642)
  32642 free blocks, 8000 free inodes, 0 directories, 8000 unused inodes
  Free blocks: 98430-131071
  Free inodes: 24001-32000
Group 4: (Blocks 131072-163839) csum 0x01a3 [INODE_UNINIT, BLOCK_UNINIT, ITABLE_ZEROED]
  Block bitmap at 130 (bg #0 + 130), csum 0x00000000
  Inode bitmap at 138 (bg #0 + 138), csum 0x00000000
  Inode table at 2142-2641 (bg #0 + 2142)
  32768 free blocks, 8000 free inodes, 0 directories, 8000 unused inodes
  Free blocks: 131072-163839
  Free inodes: 32001-40000
Group 5: (Blocks 163840-196607) csum 0x772d [INODE_UNINIT, BLOCK_UNINIT, ITABLE_ZEROED]
  Backup superblock at 163840, Group descriptors at 163841-163841
  Reserved GDT blocks at 163842-163965
  Block bitmap at 131 (bg #0 + 131), csum 0x00000000
  Inode bitmap at 139 (bg #0 + 139), csum 0x00000000
  Inode table at 2642-3141 (bg #0 + 2642)
  32642 free blocks, 8000 free inodes, 0 directories, 8000 unused inodes
  Free blocks: 163966-196607
  Free inodes: 40001-48000
Group 6: (Blocks 196608-229375) csum 0x4c9d [INODE_UNINIT, BLOCK_UNINIT, ITABLE_ZEROED]
  Block bitmap at 132 (bg #0 + 132), csum 0x00000000
  Inode bitmap at 140 (bg #0 + 140), csum 0x00000000
  Inode table at 3142-3641 (bg #0 + 3142)
  32768 free blocks, 8000 free inodes, 0 directories, 8000 unused inodes
  Free blocks: 196608-229375
  Free inodes: 48001-56000
Group 7: (Blocks 229376-255999) csum 0xa23d [INODE_UNINIT, ITABLE_ZEROED]
  Backup superblock at 229376, Group descriptors at 229377-229377
  Reserved GDT blocks at 229378-229501
  Block bitmap at 133 (bg #0 + 133), csum 0x40d7fa56
  Inode bitmap at 141 (bg #0 + 141), csum 0x00000000
  Inode table at 3642-4141 (bg #0 + 3642)
  26498 free blocks, 8000 free inodes, 0 directories, 8000 unused inodes
  Free blocks: 229502-255999
  Free inodes: 56001-64000
```

## 参考

- [ext4 disk layout](https://ext4.wiki.kernel.org/index.php/Ext4_Disk_Layout)
- [Linux 文件系统(七):磁盘布局](https://www.bilibili.com/video/BV1JT41167WS/)
- [metebalci a-minimum-complete-tutorial-of-linux-ext4-file-system](https://metebalci.com/blog/a-minimum-complete-tutorial-of-linux-ext4-file-system/)