
# ext4

文件系统中存储的最小单位是块(Block),一个块究竟多大是在格式化时确定的,例如mke2fs的-b选项可以设定块大小为1024、2048或4096字节

> 其中通过 `man mke2fs` 查看其 `-b block-size` 参数含义可知:
> 
> 该选项用于指定块的大小,单位是字节.有效的块大小值是2的幂,从 **1024** 到 **65536** (但请注意,内核只能挂载块大小小于或等于系统页大小的文件系统 - 在x86系统上是4k,在ppc64或aarch64上,根据内核配置,可以达到64k).如果省略了这个选项,块大小将根据文件系统的大小和预期用途(见-T选项)由启发式方法确定.
>
>> ext 文件系统的配置文件位于 `/etc/mke2fs.conf`
> 
> 在大多数常见情况下,默认块大小是**4k**

## EXT2 磁盘格式

### 启动块(Boot Block)

大小就是1KB,由PC标准规定,用来存储磁盘分区信息和启动信息, 任何文件系统都不能使用该块

### 超级块(Super Block)

描述整个分区的文件系统信息,例如块大小文件系统版本号、上次mount的时间等等.超级块在每个块组的开头都有一份拷贝.

### 块组描述符表(GDT)

GDT(Group Descriptor Table) 由很多块组描述符组成,整个分区分成多少个块组就对应有多少个块组描述符.每个块组描述符存储一个块组的描述信息,包括inode表哪里开始,数据块从哪里开始,空闲的inode和数据块还有多少个等.块组描述符表在每个块组的开头也都有一份拷贝,这些信息是非常重要的,因此它们都有多份拷贝.

### 块位图 (Block Bitmap)

块位图就是用来描述整个块组中哪些块已用哪些块空闲的,本身占一个块,其中的每个bit代表本块组中的一个块,这个bit为1表示该块已用,这个bit为0表示该块空闲可用

### inode位图 (inode Bitmap)

和块位图类似,本身占一个块,其中每个bit表示一个inode是否空闲可用.

### inode表 (inode Table)

文件类型(常规、目录、符号链接等),权限,文件大小,创建/修改/访问时间等信息存在inode中,每个文件都有一个inode.

### 数据块(Data Block)

- **常规文件**: 文件的数据存储在数据块中.
- **目录**: 该目录下的所有文件名和目录名存储在数据块中.(注意:文件名保存在它所在目录的数据块中,其它信息都保存在该文件的inode中)
- **符号链接**: 如果目标路径名较短则直接保存在inode中以便更快地查找,否则分配一个数据块来保存
- **设备文件、FIFO和socket等特殊文件**: 没有数据块,设备文件的主设备号和次设备号保存在inode中

## 参考

- [14.ext2文件系统](https://www.bilibili.com/video/BV1V84y1A7or/)
- [Ext4](https://en.wikipedia.org/wiki/Ext4)
- [系统性学习Ext4文件系统(图例解析)](https://zhuanlan.zhihu.com/p/476377123)
- [opensource ext4-filesystem](https://opensource.com/article/18/4/ext4-filesystem)
- [日志式文件系统(ext3)详解](https://www.cnblogs.com/yuanqiangfei/p/16932969.html)
- [Linux 文件系统 EXT4 的前世今生](https://www.oschina.net/translate/introduction-ext4-filesystem)
- [wiki ext4](https://ext4.wiki.kernel.org/index.php/Main_Page)
- [linux虚拟文件系统(二)-ext4文件系统结构](https://blog.csdn.net/sinat_22338935/article/details/119270371)
- [漫谈Linux标准的文件系统(Ext2/Ext3/Ext4)](https://www.cnblogs.com/justmine/p/9128730.html)
- [干货!大话EXT4文件系统完整版](https://cloud.tencent.com/developer/article/1551286)