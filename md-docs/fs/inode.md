
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

## 参考

- [Linux 文件系统(四):内核文件表](https://www.bilibili.com/video/BV1j24y1x7UH/)