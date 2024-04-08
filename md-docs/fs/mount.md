
# mount

```c
int mount(const char *source, const char *target,
          const char *filesystemtype, unsigned long mountflags,
          const void *data);
```

非常灵活的设计
可以把设备挂载到任何想要的位置

在Linux系统中,目录树的灵活性和强大的挂载机制是其文件系统管理的核心特点.通过挂载,可以将几乎任何类型的存储设备或文件系统添加到目录树中的任何位置,从而实现对文件的高效组织和访问.

挂载的概念允许用户将一个存储设备(无论是物理磁盘、分区,还是通过回环设备创建的虚拟磁盘)上的文件系统连接到目录树中的某个节点.这样,存储设备上的文件和目录就像本地文件系统中的一部分一样,可以通过路径轻松访问.例如,可以将一个名为`NVME0N1P1`的物理磁盘分区挂载到`/tmp/demo/FFS`目录下,使得该分区的内容在文件系统中以`/tmp/demo/FFS`的路径形式展现.

此外,Linux的启动过程也体现了文件系统挂载的强大之处.在系统启动时,initramfs(初始内存文件系统)负责扫描系统中的存储设备,并找到可启动的磁盘.一旦找到,系统会将根文件系统映射到initramfs中的某个目录,并通过`pivot_root`系统调用来切换文件系统的根目录,从而启动真正的Linux系统.

在这个过程中,`/etc/fstab`文件扮演了重要角色,它包含了系统启动时需要挂载的文件系统信息.用户可以在安装Linux系统时指定分区的挂载点,系统会根据`/etc/fstab`中的配置自动挂载这些分区.

这种灵活的挂载机制不仅使得文件管理变得简单,也为系统的维护和升级提供了便利.例如,如果需要对根文件系统进行升级或修复,可以先将其挂载到一个临时位置,然后再进行操作,而不影响系统的正常运行.

总之,Linux文件系统的挂载机制提供了一种强大而灵活的方式来管理文件和目录.通过挂载,用户可以将外部存储设备无缝集成到文件系统中,同时也为系统的启动和维护提供了极大的便利.这种设计哲学体现了Linux系统的开放性和可扩展性,使其成为广泛使用的操作系统之一.

## initramfs

Linux-minimal 运行在 "initramfs" 模式

Initial RAM file system
完整的文件系统
可以包含设备驱动等任何文件
但不具有 "持久化" 的能力

最小 "真正" Linux 的启动流程

```bash
export PATH=/bin
busybox mknod /dev/sda b 8 0
busybox mkdir -p /newroot
busybox mount -t ext2 /dev/sda /newroot
exec busybox switch_root /newroot/ /etc/init
```

通过 pivot_root (2) 实现根文件系统的切换