
# Linux from scracth

## 前言

本文用于记录 [Linux from scracth](https://www.linuxfromscratch.org/lfs/): 构建自己的 Linux 发行版 的踩坑记录

当前版本的 lfs12.0, 可以在 [downloads/stable](https://www.linuxfromscratch.org/lfs/downloads/stable/) 中找到英文资料, 笔者使用的是中文翻译版 [LFS 中文](https://lfs.xry111.site/zh_CN/)

常见问题见 [lfs FAQ](https://www.linuxfromscratch.org/faq/)

## 准备

### 准备宿主系统

笔者使用的是 Windows11 操作系统, 配合 VMware Workstation 的 Ubuntu22.04 LTS 作为宿主系统

执行下面的命令得到一个 `version-check.sh` 版本检查脚本

```bash
cat > version-check.sh << "EOF"
#!/bin/bash
# A script to list version numbers of critical development tools

# If you have tools installed in other directories, adjust PATH here AND
# in ~lfs/.bashrc (section 4.4) as well.

LC_ALL=C 
PATH=/usr/bin:/bin

bail() { echo "FATAL: $1"; exit 1; }
grep --version > /dev/null 2> /dev/null || bail "grep does not work"
sed '' /dev/null || bail "sed does not work"
sort   /dev/null || bail "sort does not work"

ver_check()
{
   if ! type -p $2 &>/dev/null
   then 
     echo "ERROR: Cannot find $2 ($1)"; return 1; 
   fi
   v=$($2 --version 2>&1 | grep -E -o '[0-9]+\.[0-9\.]+[a-z]*' | head -n1)
   if printf '%s\n' $3 $v | sort --version-sort --check &>/dev/null
   then 
     printf "OK:    %-9s %-6s >= $3\n" "$1" "$v"; return 0;
   else 
     printf "ERROR: %-9s is TOO OLD ($3 or later required)\n" "$1"; 
     return 1; 
   fi
}

ver_kernel()
{
   kver=$(uname -r | grep -E -o '^[0-9\.]+')
   if printf '%s\n' $1 $kver | sort --version-sort --check &>/dev/null
   then 
     printf "OK:    Linux Kernel $kver >= $1\n"; return 0;
   else 
     printf "ERROR: Linux Kernel ($kver) is TOO OLD ($1 or later required)\n" "$kver"; 
     return 1; 
   fi
}

# Coreutils first because-sort needs Coreutils >= 7.0
ver_check Coreutils      sort     7.0 || bail "--version-sort unsupported"
ver_check Bash           bash     3.2
ver_check Binutils       ld       2.13.1
ver_check Bison          bison    2.7
ver_check Diffutils      diff     2.8.1
ver_check Findutils      find     4.2.31
ver_check Gawk           gawk     4.0.1
ver_check GCC            gcc      5.1
ver_check "GCC (C++)"    g++      5.1
ver_check Grep           grep     2.5.1a
ver_check Gzip           gzip     1.3.12
ver_check M4             m4       1.4.10
ver_check Make           make     4.0
ver_check Patch          patch    2.5.4
ver_check Perl           perl     5.8.8
ver_check Python         python3  3.4
ver_check Sed            sed      4.1.5
ver_check Tar            tar      1.22
ver_check Texinfo        texi2any 5.0
ver_check Xz             xz       5.0.0
ver_kernel 4.14

if mount | grep -q 'devpts on /dev/pts' && [ -e /dev/ptmx ]
then echo "OK:    Linux Kernel supports UNIX 98 PTY";
else echo "ERROR: Linux Kernel does NOT support UNIX 98 PTY"; fi

alias_check() {
   if $1 --version 2>&1 | grep -qi $2
   then printf "OK:    %-4s is $2\n" "$1";
   else printf "ERROR: %-4s is NOT $2\n" "$1"; fi
}
echo "Aliases:"
alias_check awk GNU
alias_check yacc Bison
alias_check sh Bash

echo "Compiler check:"
if printf "int main(){}" | g++ -x c++ -
then echo "OK:    g++ works";
else echo "ERROR: g++ does NOT work"; fi
rm -f a.out
EOF

bash version-check.sh
```

执行脚本, 绝大部分是正常的, 但是出现了两条 error

```bash
ERROR: /bin/sh does not point to bash
version-check.sh: line 41: makeinfo: command not found
```

为了解决第一个 /bin/sh 不指向 bash 而是 dash 可以执行如下的命令

```bash
sudo dpkg-reconfigure dash
```

此时会弹出来一个框, 然后选择 no 即可

![20230701211216](https://raw.githubusercontent.com/learner-lu/picbed/master/20230701211216.png)

> 解决办法: [how-to-use-bash-for-sh-in-ubuntu](https://unix.stackexchange.com/questions/442510/how-to-use-bash-for-sh-in-ubuntu)

makeinfo 的问题安装 textinfo 即可

```bash
sudo apt install texinfo
```

> 解决办法: [what is makeinfo and how do i get it](https://stackoverflow.com/questions/338317/what-is-makeinfo-and-how-do-i-get-it)

读者可能还有一些其他的问题,比如 bison/gakw not found 之类, 也是 apt install 就可以解决

接下来执行没有任何问题, 笔者的软件版本号如下:

```bash
bash version-check.sh
OK:    Coreutils 8.32   >= 7.0
OK:    Bash      5.1.16 >= 3.2
OK:    Binutils  2.38   >= 2.13.1
OK:    Bison     3.8.2  >= 2.7
OK:    Diffutils 3.8    >= 2.8.1
OK:    Findutils 4.8.0  >= 4.2.31
OK:    Gawk      5.1.0  >= 4.0.1
OK:    GCC       11.3.0 >= 5.1
OK:    GCC (C++) 11.3.0 >= 5.1
OK:    Grep      3.7    >= 2.5.1a
OK:    Gzip      1.10   >= 1.3.12
OK:    M4        1.4.18 >= 1.4.10
OK:    Make      4.3    >= 4.0
OK:    Patch     2.7.6  >= 2.5.4
OK:    Perl      5.34.0 >= 5.8.8
OK:    Python    3.10.6 >= 3.4
OK:    Sed       4.8    >= 4.1.5
OK:    Tar       1.34   >= 1.22
OK:    Texinfo   6.8    >= 5.0
OK:    Xz        5.2.5  >= 5.0.0
OK:    Linux Kernel 5.15.133.1 >= 4.14
OK:    Linux Kernel supports UNIX 98 PTY
Aliases:
OK:    awk  is GNU
OK:    yacc is Bison
OK:    sh   is Bash
Compiler check:
OK:    g++ works
```

接下来需要添加一个新硬盘, 这里添加了一块 40GB 的 SCSI 格式的新硬盘, 需要确保虚拟设备位于 SCSI 0:1, 也就是 `/dev/sda`

![20231202184919](https://raw.githubusercontent.com/learner-lu/picbed/master/20231202184919.png)

使用 `fdisk` 创建分区, 这里只创建了 `boot` `swap` `/` 分区

> 在Linux系统中,**交换空间(swap space)是一块磁盘空间,用作物理内存(RAM)的延伸,以便在系统内存不足时存储临时数据。**. 交换空间的目的是在物理内存(RAM)不足时提供额外的虚拟内存空间,以避免系统因内存不足而出现性能问题或崩溃。一般来说, **建议将交换空间的大小设置为物理内存大小的两倍**

![1190a](https://raw.githubusercontent.com/learner-lu/picbed/master/1190a.gif)

使用如下命令可以看到原先的 `/dev/sda` 已经被划分为三个分区了, `/dev/sdb` 是原先的操作系统磁盘

![QQ截图20231202184124](https://raw.githubusercontent.com/learner-lu/picbed/master/QQ截图20231202184124.png)

需要注意的是, 这里的 **swap 分区位于 /dev/sda2, root 分区位于 /dev/sda3**, 这两个分区的路径需要记住, 后文挂载的时候需要按照这个路径执行

```bash
## 制作根文件系统 (/) 采⽤ ext4 文件系统
kamilu@kamilu:~$ sudo mkfs -v -t ext4 /dev/sda3

## 初始化新 swap 分区
kamilu@kamilu:~$ sudo mkswap /dev/sda2
```

严格来说,**我们不能"挂载一个分区".我们挂载的是该分区中的文件系统**.但是,由于一个分区最多只包含一个文件系统,人们经常不加区分地用"分区"代表分区中的文件系统。

在 ~/.bashrc 和 /root/.bashrc 中添加 LFS 环境变量, 这样每次启动 bash 的时候都会有这个环境变量

```bash
## 设置 LFS 环境变量
export LFS=/mnt/lfs
```

输入以下命令以创建挂载点,并挂载 LFS 文件系统:

```bash
mkdir -pv $LFS
mount -v -t ext4 /dev/sda3 $LFS

## 使用 swapon 命令启用 swap 分区
/sbin/swapon -v /dev/sda2
```

现在可以使用 lsblk 看到 /dev/sda 的 swap 和 / 分区有挂载点了

```bash
root@kamilu:/home/kamilu# lsblk
NAME   MAJ:MIN RM   SIZE RO TYPE MOUNTPOINTS
sda      8:0    0    40G  0 disk
├─sda1   8:1    0   200M  0 part
├─sda2   8:2    0     4G  0 part [SWAP]
└─sda3   8:3    0  35.8G  0 part /mnt/lfs
sdb      8:16   0    80G  0 disk
├─sdb1   8:17   0     1M  0 part
├─sdb2   8:18   0   513M  0 part /boot/efi
└─sdb3   8:19   0  79.5G  0 part /
```

### 软件包和补丁

> 软件包备用地址: [linuxfromscratch mirrors](https://www.linuxfromscratch.org/lfs/mirrors.html#files)

```bash
wget https://lfs.xry111.site/zh_CN/12.0/wget-list-sysv
wget --input-file=wget-list-sysv --continue --directory-prefix=$LFS/sources
```  

expat 下载失败了

```bash
wget https://mirror-hk.koddos.net/lfs/lfs-packages/12.1/expat-2.6.0.tar.xz
```

> [lfs package mirror](https://mirror-hk.koddos.net/lfs/lfs-packages/12.1/)

### 最后准备工作

创建所需的目录布局

```bash
mkdir -pv $LFS/{etc,var} $LFS/usr/{bin,lib,sbin}

for i in bin lib sbin; do
  ln -sv usr/$i $LFS/$i
done

case $(uname -m) in
  x86_64) mkdir -pv $LFS/lib64 ;;
esac
mkdir -pv $LFS/tools
```

创建并以 lfs 用户继续操作

```bash
groupadd lfs
useradd -s /bin/bash -g lfs -m -k /dev/null lfs
passwd lfs
chown -v lfs $LFS/{usr{,/*},lib,var,etc,bin,sbin,tools}
case $(uname -m) in
  x86_64) chown -v lfs $LFS/lib64 ;;
esac
su - lfs
```

初始的 shell 是一个登录 shell.它读取宿主系统的 `/etc/profile` 文件 (可能包含一些设置和环境变量),然后读取 `.bash_profile`.

我们在 `.bash_profile` 中使用 `exec env -i.../bin/bash` 命令,新建一个除了 HOME, TERM 以及 PS1 外没有任何环境变量的 shell 并替换当前 shell.这可以防止宿主环境中不需要和有潜在风险的环境变量进入构建环境

新的 shell 实例是 **非登录** shell,它不会读取和执行 /etc/profile 或者 .bash_profile 的内容,而是读取并执行 .bashrc 文件

```bash
set +h
umask 022
LFS=/mnt/lfs
LC_ALL=POSIX
LFS_TGT=$(uname -m)-lfs-linux-gnu
PATH=/usr/bin
if [ ! -L /bin ]; then PATH=/bin:$PATH; fi
PATH=$LFS/tools/bin:$PATH
CONFIG_SITE=$LFS/usr/share/config.site
export LFS LC_ALL LFS_TGT PATH CONFIG_SITE
```

> set +h
> 
> set +h 命令关闭 bash 的散列功能。一般情况下,散列是很有用的。**bash 使用一个散列表维护各个可执行文件的完整路径,这样就不用每次都在 PATH 指定的目录中搜索可执行文件**.然而,在构建 LFS 时,我们希望总是使用最新安装的工具。**关闭散列功能强制 shell 在运行程序时总是搜索 PATH.这样,一旦$LFS/tools/bin 中有新的工具可用,shell 就能够找到它们,而不是使用之前记忆在散列表中,由宿主发行版提供的 /usr/bin 或 /bin 中的工具。**

一些商业发行版(比如 Ubuntu) 未做文档说明地将 /etc/bash.bashrc 引入 bash 初始化过程。该文件可能修改 lfs 用户的环境,并影响 LFS 关键软件包的构建。为了保证 lfs 用户环境的纯净,检查 /etc/bash.bashrc 是否存在,如果它存在就将它移走。以 root 用户身份,运行:

```bash
$ su
$ [ ! -e /etc/bash.bashrc ] || mv -v /etc/bash.bashrc /etc/bash.bashrc.NOUSE
```

当用户登录时, Bash shell 会读取并执行 bash_profile 文件,以确保用户在命令行下有一个符合其需求的环境。需要注意的是,bash_profile 通常是针对单个用户的配置文件,每个用户都可以有自己的 bash_profile 文件。

在某些系统中,也可以使用 bashrc 文件来实现类似的配置,但其作用范围略有不同。**bash_profile 通常在用户登录时执行一次,而 bashrc 在每次新打开一个终端窗口时都会执行。**

## 构建 LFS 交叉工具链和临时工具

## 构建 LFS 系统/

LFS 被设计为在一次会话中构建完成。换句话说,本书的指令假设,在整个编译过程中,系统不会关闭或重启。当然,构建过程不需要严格地一气呵成,但是要注意如果在重新启动后继续编译 LFS,根据构建进度的不同,可能需要再次进行某些操作。

先用 lsblk 确定分区的位置, 假设为 `/dev/sdb2` 的 swap, `/dev/sdb3` 的 /

> 笔者之前是 sda, 现在不知道怎么变成 sdb 了?

```bash
mount -v -t ext4 /dev/sdb3 $LFS
mount -v -t ext4 /dev/sdb1 $LFS/boot
/sbin/swapon -v /dev/sdb2
```

```bash
# 为了在任何宿主系统上都能填充 $LFS/dev,只能绑定挂载宿主系统的 /dev 目录。绑定挂载是一种特殊挂载类型,它允许通过不同的位置访问一个目录树或一个文件
mount -v --bind /dev $LFS/dev

# 挂载其余的虚拟内核文件系统
mount -v --bind /dev/pts $LFS/dev/pts
mount -vt proc proc $LFS/proc
mount -vt sysfs sysfs $LFS/sys
mount -vt tmpfs tmpfs $LFS/run

if [ -h $LFS/dev/shm ]; then
  mkdir -pv $LFS/$(readlink $LFS/dev/shm)
else
  mount -t tmpfs -o nosuid,nodev tmpfs $LFS/dev/shm
fi
```

```bash
chroot "$LFS" /usr/bin/env -i   \
    HOME=/root                  \
    TERM="$TERM"                \
    PS1='(lfs chroot) \u:\w\$ ' \
    PATH=/usr/bin:/usr/sbin     \
    /bin/bash --login
```

退出

```bash
logout
```

```bash
umount -v $LFS/dev/pts
mountpoint -q $LFS/dev/shm && umount $LFS/dev/shm
umount -v $LFS/dev
umount -v $LFS/run
umount -v $LFS/proc
umount -v $LFS/sys
umount -v $LFS/boot
umount -l $LFS
swapoff -v /dev/sdb2
```

> 这里使用 `umount -v $LFS` 会出现 target is buzy 的提示, 因此换成 `-l` ,指令将会自动解决冲突的问题

![20240302111116](https://raw.githubusercontent.com/learner-lu/picbed/master/20240302111116.png)

```bash
/dev/sdb2: UUID="cb057e3f-5dab-40e5-b59c-6edf00586212" TYPE="swap" PARTUUID="fede849a-89b5-7c43-a11c-f657c82aa412"
/dev/sdb3: UUID="a0a70601-6c80-4442-8f8b-c0e22d6c5aa1" BLOCK_SIZE="4096" TYPE="ext4" PARTUUID="c1869f3b-d111-2c43-a249-765ed0106ddd"
/dev/sdb1: UUID="ac489137-383a-4bee-88dd-0954dbed35ad" BLOCK_SIZE="1024" TYPE="ext4" PARTUUID="03e1d974-71f4-b847-871d-27b3fc15baa4"
```

## 参考

- [LFS 中文](https://lfs.xry111.site/zh_CN/)
- [how-to-use-bash-for-sh-in-ubuntu](https://unix.stackexchange.com/questions/442510/how-to-use-bash-for-sh-in-ubuntu)
- [what is makeinfo and how do i get it](https://stackoverflow.com/questions/338317/what-is-makeinfo-and-how-do-i-get-it)
- [LFS 11.2(Linux From Scratch)构建过程全记录(二):磁盘分区](https://www.cnblogs.com/alphainf/p/16663371.html)
- [LFS 11.2(Linux From Scratch)构建过程全记录(十): 使 LFS 系统可引导](https://www.cnblogs.com/alphainf/p/16720497.html)
- [<从LFS到自己的Linux发行版>系列教程:一步到位体验LFS11.0](https://www.cnblogs.com/hzmanage/p/15744414.html)