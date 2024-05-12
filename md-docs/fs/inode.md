
# inode

## inode 内容

Unix文件系统的核心抽象是inode和目录项(directory entry).inode相当于文件的元数据索引,存储在磁盘上的一个文件中,其索引即inode表中的下标.

例如,在ext文件系统中,第2号inode通常对应根目录, inode包含的信息主要有:

1. **文件类型和权限(mode+premission)**:文件是目录、普通文件、链接文件,还是特殊设备文件等.
2. **文件大小(size in bytes)**:文件所占字节数.
3. **引用计数(nlink)**:文件被引用的次数,计数为零时inode可以被释放.
4. **块指针(block address)**:文件数据存放的磁盘块编号,可能包括直接块指针、间接块指针、双重间接和三重间接块指针.

需要注意的是**文件名不存储在inode中,而是存储在目录的inode中**,因为文件可能对应多个文件名.改变文件名时,实际上是在修改目录的inode,而非文件本身的inode.这表明,目录和普通文件在文件系统级别上都是inode,但它们代表不同的概念.**目录可以视为一种特殊的文件**,其中包含了指向其他文件或目录的文件名和对应的inode索引.

除了上述必要的一些 inode 信息外, 还可以选择保存一些其他信息, 比如存储文件的时间戳信息,包括访问时间(access time)、创建时间(creation time)和修改时间(modification time).其中,修改时间是最常见的.然而,考虑到闪存存储介质的写入次数有限,许多现代文件系统在挂载时可以选择不更新访问时间,以减少不必要的磁盘写入.此外,文件系统还会存储文件的所有者信息和所属组的ID,通常以32位整数表示.对于UNIX文件系统,还可能包含设备号等其他元数据,但这些与文件核心操作关系不大.

> 例如对于一些只读的文件系统, 大量的读还需要修改文件的访问时间, 产生写操作, 因此为了避免不必要的磁盘写入操作可以选择在挂载时选择只读
>
> ```bash
> mount -o ro /dev/mydisk /mnt/mydisk
> ```
>
> 添加 `noatime` 忽略更新 access time
>
> ```bash
> mount -o ro,noatime /dev/mydisk /mnt/mydisk
> ```

在同一个文件系统中, inode number 是唯一的, 硬链接(hard-link)不能跨文件系统, 软链接(soft-link/symbolic link)可以

## ext2

我们先来介绍一下 ext2 的 inode 设计, 虽然目前说这已经是一种过时的设计了, ~~但是很有意思~~

完整的 ext2 inode 结构如下, 其中 `__le` 开头的代表小端存储的数据

```c
struct ext2_inode {
    __le16 i_mode;        /* 文件模式 */
    __le16 i_uid;         /* 所有者用户ID的低16位 */
    __le32 i_size;        /* 以字节为单位的文件大小 */
    __le32 i_atime;       /* 最后访问时间 */
    __le32 i_ctime;       /* 创建时间 */
    __le32 i_mtime;       /* 最后修改时间 */
    __le32 i_dtime;       /* 删除时间 */
    __le16 i_gid;         /* 组ID的低16位 */
    __le16 i_links_count; /* 链接计数 */
    __le32 i_blocks;      /* 块计数 */
    __le32 i_flags;       /* 文件标志 */
    union {
        struct {
            __le32 l_i_reserved1;
        } linux1;
        struct {
            __le32 h_i_translator;
        } hurd1;
        struct {
            __le32 m_i_reserved1;
        } masix1;
    } osd1;                        /* 操作系统依赖1 */
    __le32 i_block[EXT2_N_BLOCKS]; /* 指向块的指针 */
    __le32 i_generation;           /* 文件版本(用于NFS) */
    __le32 i_file_acl;             /* 文件ACL */
    __le32 i_dir_acl;              /* 目录ACL */
    __le32 i_faddr;                /* 文件碎片地址 */
    union {
        struct {
            __u8 l_i_frag;  /* 碎片编号 */
            __u8 l_i_fsize; /* 碎片大小 */
            __u16 i_pad1;
            __le16 l_i_uid_high; /* 这两个字段 */
            __le16 l_i_gid_high; /* 是reserved2[0] */
            __u32 l_i_reserved2;
        } linux2;
        struct {
            __u8 h_i_frag;  /* 碎片编号 */
            __u8 h_i_fsize; /* 碎片大小 */
            __le16 h_i_mode_high;
            __le16 h_i_uid_high;
            __le16 h_i_gid_high;
            __le32 h_i_author;
        } hurd2;
        struct {
            __u8 m_i_frag;  /* 碎片编号 */
            __u8 m_i_fsize; /* 碎片大小 */
            __u16 m_pad1;
            __u32 m_i_reserved2[2];
        } masix2;
    } osd2; /* 操作系统依赖2 */
};
```

> 位于源码中 [fs/ext2/ext2.h](https://github.com/luzhixing12345/klinux/blob/0abe96b48167520ec079828b2ee341a100eb416d/fs/ext2/ext2.h#L290-L342)
> 
> 字段含义见 [ext2 inode table](https://www.nongnu.org/ext2-doc/ext2.html#inode-table)

其中可以计算得到 sizeof(struct ext2_inode) = 128. inode 简化后的字段如下所示, 其中 mode/uid/timestamps 等元数据字段前文已经介绍过了, 最重要的是这里的 `i_block` 字段, 这是一个长度为 15 的数组, 前 12 位为直接引用块, 每一个分别对应一个 data block 的 block_number. 后3位分别是1/2/3级间接引用块. 间接引用块指向的块全部用来保存 block_number, 在 ext2 时代 block_number 为 32 位, 因此每一个 block 可以保存 256 个 block_number, 二级和三级同理.

![20240509201555](https://raw.githubusercontent.com/learner-lu/picbed/master/20240509201555.png)

稍微一计算可以得出, 如果全部占满的情况下, 最大的文件大小为 12 + 256 + 256x256 + 256x256x256 ~= 16GB. `ext2_inode` 结构体还有一个字段为 `i_blocks` 用于记录该文件占用的块个数, 由于各级间接块的下属子块数量是固定的, 因此可以根据块个数判断出来使用了哪些部分, 也可以根据文件大小来计算应该使用几级间接引用块来保存

> 比如 269, 那么就是 12 + 256 + 1, 即 12 Direct Block + 1 Indirect Block + 1 Double Block 的第一个子块的第一项
>
> 同理如果要 seek(1MB) 的位置也可以通过相同的方式找到对应的块

ext2 非常适合小文件(12KB)以内的文件, 因为只需要查找到 inode 即可从中读取出所有数据块的 block_number, 然后就可以通过偏移量找到对应的地址了; 如果再大的文件那么就会因为多几次间接查找带来一定的开销

---

ext2 的 inode 结构我们可以看出, 文件的追加很容易, 但是插入很麻烦(因为需要整体移动). 文件大小 != 占用磁盘大小, 大多数情况下占用的磁盘大小要比文件大小大(因为会有空余的块). 但是也可能相反, 比如说对于一个稀疏文件, 即 lseek(1MB) + write(1KB)

例如我们使用 `dd` 生成了一个 1000MB 的全0文件, 然后在某一个偏移量的地方写入了几个字节, 此时再使用 `du` 查看时就会发现文件大小要小很多

> 因为 ext4 中可以延迟写入, 对于全 0 的块可以暂时不分配 data block, 直到写入非 0 值时才进行更新

![20240509221631](https://raw.githubusercontent.com/learner-lu/picbed/master/20240509221631.png)

## ext4

在ext4文件系统中,传统的间接块(indirect block)已被废弃,转而使用扩展树(extend tree,其中"extend"在此处可译为"扩展")来提高大文件性能并减少文件碎片.

扩展树通过将连续的物理块视为一段扩展(extent),简化了传统文件系统中使用单次、二次和三次间接块映射的方式.例如,一个文件的200个连续块可以仅通过一条记录 `(0, 200)` 来表示,而不是在间接块或双重间接块中逐个列出.

其中 `ext4_extent` 的数据结构如下所示

```c
struct ext4_extent {
    __le32  ee_block;       /* 文件逻辑块号的第一个块,表示这个扩展的起始块 */
    __le16  ee_len;         /* 这个扩展覆盖的数据块数量 */
    __le16  ee_start_hi;    /* 物理块号的高 16 位 */
    __le32  ee_start_lo;    /* 物理块号的低 32 位 */
};
```

根据 [ext4 文档](https://kernel.org/doc/html/v6.6/filesystems/ext4/dynamic.html#extent-tree) 信息可知, 其中 `ee_len` 字段用于表示范围覆盖的块数. 如果此字段的值为 <= 32768,则认为该块没有被使用. 如果字段的值为 > 32768,则数据块正在使用,实际数据块长度为 `ee_len - 32768`. 因此计算得到扩展块的最大大小为 2^(16-1) x 2^12(4KB) = 128MB, 因此 **ext4 中的单个扩展块最多可以映射 128 MiB 的连续空间**, 可以看出**ext4 对于连续分配的大文件比较友好**

ext4 是 ext2 兼容的, 所有 ext2 inode 具有的数据结构成员 ext4 都具备. 同样采用 `i_block[15]` 来保存文件的数据, 但与 ext2 的所采用的间接引用块不同的是, 这60个字节(4*15)用来保存 4 个 `ext4_extent` 和 1 个 `ext4_extent_header`

> sizeof(ext4_extent) = 12
> 
> sizeof(ext4_extent_header) = 12
>
> 12x5 = 60

```c
struct ext4_extent_header {
    __le16 eh_magic;      /* 魔数,用于识别扩展索引的格式 */
    __le16 eh_entries;    /* 有效的条目数量,表示当前有多少条目在使用 */
    __le16 eh_max;        /* 存储空间的最大容量,以条目数计 */
    __le16 eh_depth;      /* 树的深度,表示是否有实际的底层块 */
    __le32 eh_generation; /* 扩展索引树的版本号,用于确保一致性 */
};
```

因此一个 inode 中可以直接保存四个 extent 结构. 如果有超过 4 个 extent, 此时扩展索引可以形成一个树状结构. 

- `eh_magic`:魔数,用于快速识别和验证数据结构的类型.在扩展索引中,这个字段通常包含一个特定的值,以帮助文件系统识别和处理扩展索引数据.
- `eh_entries`:当前索引树中有效条目的数量.这个值表示有多少个`ext4_extent`条目被用来存储文件的块映射信息.
- `eh_max`:索引树存储空间的最大容量,以条目数计.这个值定义了索引树可以存储的`ext4_extent`条目数量的最大值.
- `eh_depth`:索引树的深度.在ext4文件系统中,扩展索引可以形成一个树状结构,这个字段表示树的层次.如果一个索引树只有一个头节点而没有分支节点,那么它的深度为0;如果树有分支节点,则深度为1或更大.
- `eh_generation`:扩展索引树的版本号.这个字段用于跟踪索引树的变更,以确保在并发操作中保持数据的一致性.每当索引树发生变化时,这个值会增加,帮助系统检测到不一致的状态.

通过这个头部信息,文件系统可以了解扩展索引树的状态,包括它的大小、容量和版本,从而有效地管理和访问大型文件的数据.

## 参考

- [Linux 文件系统(四):内核文件表](https://www.bilibili.com/video/BV1j24y1x7UH/)
- [ext2 格式](https://www.nongnu.org/ext2-doc/ext2.html)
