
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

> 在Linux系统中,**交换空间(swap space)是一块磁盘空间,用作物理内存(RAM)的延伸,以便在系统内存不足时存储临时数据.**. 交换空间的目的是在物理内存(RAM)不足时提供额外的虚拟内存空间,以避免系统因内存不足而出现性能问题或崩溃. 一般来说, **建议将交换空间的大小设置为物理内存大小的两倍**

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

严格来说,**我们不能"挂载一个分区".我们挂载的是该分区中的文件系统**.但是,由于一个分区最多只包含一个文件系统,人们经常不加区分地用"分区"代表分区中的文件系统.

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

- **Acl**: 管理访问控制列表 (ACL) 的工具,用来对文件和目录提供细粒度的访问权限控制.
- **Attr**: 管理文件系统对象的扩展属性的程序.
- **Autoconf**: 提供能根据软件开发者提供的模板,自动生成配置源代码的 shell 脚本的程序.如果修改了软件包的构建过程,一般需要该软件包的支持才能重新构建被修改的软件包.
- **Automake**: 包含能根据软件开发者提供的模板,自动生成 Makefile 的程序.如果修改了软件包的构建过程,一般需要该软件包的支持才能重新构建被修改的软件包.
- **Bash**: 为系统提供一个 LSB core 要求的 Bourne Shell 接口.它是较为常用的 shell 软件包,且具有一定的的扩展能力,因此在各种 shell 软件包中选择了它.
- **Bc**: 提供了一个任意精度数值处理语言.在编译 Linux 内核时需要该软件包.
- **Binutils**: 提供链接器/汇编器,以及其他处理目标文件的工具.编译 LFS 系统中的大多数软件包都需要这些程序.
- **Bison**: 包含 yacc (Yet Another Compiler Compiler) 的 GNU 版本.一些 LFS 程序的编译过程需要该软件包.
- **Bzip2**: 用于压缩和解压缩文件的程序.许多 LFS 软件包的解压需要该软件包.
- **Check**: 提供其他程序使用的测试环境.
- **Coreutils**: 包含一些用于查看和操作文件和目录的基本程序.这些程序被用于在命令行下管理文件,以及每个 LFS 软件包的安装过程.
- **DejaGNU**: 提供用于测试其他程序的框架.
- **Diffutils**: 包含用于显示文件或目录之间的差异的程序.这些程序可以被用于创建补丁,很多软件包的编译过程也需要该软件包.
- **E2fsprogs**: 提供用于处理 ext2, ext3 和 ext4 文件系统的工具.它们是 Linux 支持的最常用且久经考验的文件系统.
- **Expat**: 提供一个相对轻量级的 XML 解析库.Perl 模块 XML::Parser 需要该软件包.
- **Expect**: 包含一个自动和其他交互程序交互的脚本执行程序.一般用它测试其他程序.
- **File**: 包含用于判定给定文件类型的工具.一些软件包的构建脚本需要它.
- **Findutils**: 提供用于在文件系统中寻找文件的程序.它被许多软件包的编译脚本使用.
- **Flex**: 包含用于生成词法分析器的程序.它是 lex (lexical analyzer) 程序的 GNU 版本.许多 LFS 软件包的编译过程需要该软件包.
- **Gawk**: 提供用于操作文本文件的程序.它是 awk (Aho-Weinberg-Kernighan) 的 GNU 版本.它被许多其他软件包的构建脚本使用.
- **GCC**: GNU 编译器的集合.它包含 C 和 C++ 编译器,以及其他一些在 LFS 中不会涉及的编译器.
- **GDBM**: 包含 GNU 数据库管理库.LFS 的另一个软件包 Man-DB 需要该软件包.
- **Gettext**: 提供用于许多其他软件包的国际化和本地化的工具和库
- **Glibc**: 包含主要的 C 语言库.Linux 程序没有该软件包的支持根本无法运行
- **GMP**: 提供数学库,这些库支持用于任意精度算术的函数.编译 GCC 需要该软件包
- **Gperf**: 提供一个能够根据键值集合生成完美散列函数的程序.Udev 需要该软件包
- **Grep**: 包含在文本中搜索指定模式的程序.它被多数软件包的编译脚本所使用
- **Groff**: 提供用于处理和格式化文本的程序.它们的一项重要功能是格式化 man 页面
- **GRUB**: Grand Unified Boot Loader.Linux 可以使用其他引导加载器,但 GRUB 最灵活
- **Gzip**: 包含用于压缩和解压缩文件的程序.许多 LFS 软件包的解压需要该软件包
- **Iana-etc**: 包含网络服务和协议的描述数据.网络功能的正确运作需要该软件包
- **Inetutils**: 提供基本网络管理程序
- **Intltool**: 提供能够从源代码中提取可翻译字符串的工具
- **IProute2**: 提供了用于 IPv4 和 IPv6 网络的基础和高级管理程序.和另一个常见的网络工具包 net-tools 相比,它具有管理 IPv6 网络的能力
- **Kbd**: 提供键盘映射文件,用于非美式键盘的键盘工具,以及一些控制台字体.
- **Kmod**: 提供用于管理 Linux 内核模块的程序
- **Less**: 包含一个很好的文本文件查看器,它支持在查看文件时上下滚动.许多软件包使用它对输出进行分页
- **Libcap**: 实现了用于访问 Linux 内核中 POSIX 1003.1e 权能字功能的用户空间接口
- **Libelf**: 提供了用于 ELF 文件和 DWARF 数据的工具和库.该软件包的大多数工具已经由其他软件包提供,但使用默认 (也是最高效的) 配置构建 Linux 内核时,需要使用该软件包的库
- **Libffi**: 实现了一个可移植的高级编程接口,用于处理不同的调用惯例.某些程序在编译时并不知道如何向函数传递参数,例如解释器在运行时才得到函数的参数个数和类型信息.它们可以使用 libffi 作为解释语言和编译语言之间的桥梁.
- **Libpipeline**: 提供一个能够灵活/方便地操作子进程流水线的库.Man-DB 软件包需要这个库.
- **Libtool**: 包含 GNU 通用库支持脚本.它将共享库的使用封装成一个一致/可移植的接口.在其他 LFS 软件包的测试套件中需要该软件包.
- **Libxcrypt**: 一些软件包 (例如 Shadow) 使用该库对密码进行散列操作.它替代 Glibc 中过时的 libcrypt 实现
- **Linux Kernel**: 我们平常说的 "GNU/Linux" 环境中的 "Linux" 就指的是它.
- **M4**: 提供通用的文本宏处理器.它被其他程序用作构建工具.
- **Make**: 用于指导软件包编译过程的程序.LFS 中几乎每个软件包都需要它.
- **Man-DB**: 包含用于查找和浏览 man 页面的程序.与 man 软件包相比,该软件包的国际化功能更为强大.该软件包提供了 man 程序.
- **Man-pages**: 提供基本的 Linux man 页面的实际内容.
- **Meson**: 一个自动化软件构建过程的工具.它的设计目标是最小化软件开发者不得不用于配置构建系统的时间.该软件包在构建 Systemd 和很多 BLFS 软件包时是必要的.
- **MPC**: 提供用于复数算术的函数.GCC 需要该软件包.
- **MPFR**: 包含用于多精度算术的函数.GCC 需要该软件包.
- **Ninja**: 提供一个注重执行速度的小型构建系统.它被设计为读取高级构建系统生成的输入文件,并以尽量高的速度运行.Meson 需要该软件包.
- **Ncurses**: 包含用于处理字符界面的不依赖特定终端的库.它一般被用于为菜单系统提供光标控制.一些 LFS 软件包需要该软件包.
- **Openssl**: 包含关于密码学的管理工具和库,它们为 Linux 内核等其他软件包提供密码学功能.
- **Patch**: 包含一个通过 补丁 文件修改或创建文件的程序.补丁文件通常由 diff 程序创建.一些 LFS 软件包的编译过程需要该软件包.
- **Perl**: 是运行时语言 PERL 的解释器.几个 LFS 软件包的安装和测试过程需要该软件包.
- **Pkgconf**: 包含一个为开发库配置编译和链接选项的程序.该程序可以直接替代 pkg-config 命令,许多软件包的构建系统都需要该命令.它的维护比原始的 Pkg-config 软件包更积极,而且运行速度稍快一些.
- **Procps-NG**: 包含用于监控系统进程的程序,对系统管理非常有用.另外 LFS 引导脚本也需要该软件包.
- **Psmisc**: 提供一些显示当前运行的系统进程信息的程序.这些程序对系统管理非常有用.
- **Python** 3: 提供了一种在设计时强调代码可读性的解释性语言支持.
- **Readline**: 是一组库,提供命令行编辑和历史记录支持.Bash 需要该软件包.
- **Sed**: 可以在没有文本编辑器的情况下编辑文本文件.另外,许多 LFS 软件包的配置脚本需要该软件包.
- **Shadow**: 包含用于安全地处理密码的程序.
- **Sysklogd**: 提供用于记录系统消息的程序,这些消息包括内核或者守护进程在异常事件发生时的提示.
- **Sysvinit**: 提供init程序,在 Linux 系统中它是其他所有进程的祖先.
- **Udev**: 是一个设备管理器,它随着系统中硬件设备的增加或移除,动态地控制 /dev 目录中设备节点的所有权,访问权限,文件名,以及符号链接.
- **Tar**: 提供存档和提取功能,几乎每个 LFS 软件包都需要它才能被提取.
- **Tcl**: 包含在测试套件中广泛使用的工具控制语言 (Tool Command Language).
- **Texinfo**: 提供用于阅读/编写和转换 info 页面的程序.许多 LFS 软件包的安装过程需要使用它.
- **Util**-linux: 包含许多工具程序,其中有处理文件系统/终端/分区和消息的工具.
- **Vim**: 提供一个编辑器.由于它与经典的 vi 编辑器相兼容,且拥有许多强大的功能,我们选择这个编辑器.编辑器的选择是非常主观的.如果希望的话,读者可以用其他编辑器替代它.
- **Wheel**: 该软件包提供一个 Python 模块,该模块是 Python wheel 软件包标准格式的参考实现.
- **XML::Parser**: 是和 Expat 交互的 Perl 模块.
- **XZ Utils**: 包含用于压缩和解压缩文件的程序.在所有这类程序中,该软件包提供了最高的压缩率.该软件包被用于解压 XZ 或 LZMA 格式的压缩文件.
- **Zlib**: 包含一些程序使用的压缩和解压缩子程序.
- **Zstd**: 提供一些程序使用的压缩和解压缩子程序.它具有较高的压缩比,以及很宽的压缩比/速度权衡范围.

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
> set +h 命令关闭 bash 的散列功能.一般情况下,散列是很有用的. **bash 使用一个散列表维护各个可执行文件的完整路径,这样就不用每次都在 PATH 指定的目录中搜索可执行文件**.然而,在构建 LFS 时,我们希望总是使用最新安装的工具.**关闭散列功能强制 shell 在运行程序时总是搜索 PATH.这样,一旦$LFS/tools/bin 中有新的工具可用,shell 就能够找到它们,而不是使用之前记忆在散列表中,由宿主发行版提供的 /usr/bin 或 /bin 中的工具.**

一些商业发行版(比如 Ubuntu) 未做文档说明地将 /etc/bash.bashrc 引入 bash 初始化过程.该文件可能修改 lfs 用户的环境,并影响 LFS 关键软件包的构建.为了保证 lfs 用户环境的纯净,检查 /etc/bash.bashrc 是否存在,如果它存在就将它移走.以 root 用户身份,运行:

```bash
$ su
$ [ ! -e /etc/bash.bashrc ] || mv -v /etc/bash.bashrc /etc/bash.bashrc.NOUSE
```

当用户登录时, Bash shell 会读取并执行 bash_profile 文件,以确保用户在命令行下有一个符合其需求的环境.需要注意的是,bash_profile 通常是针对单个用户的配置文件,每个用户都可以有自己的 bash_profile 文件.

在某些系统中,也可以使用 bashrc 文件来实现类似的配置,但其作用范围略有不同.**bash_profile 通常在用户登录时执行一次,而 bashrc 在每次新打开一个终端窗口时都会执行.**

## 参考

- [LFS 中文](https://lfs.xry111.site/zh_CN/)
- [how-to-use-bash-for-sh-in-ubuntu](https://unix.stackexchange.com/questions/442510/how-to-use-bash-for-sh-in-ubuntu)
- [what is makeinfo and how do i get it](https://stackoverflow.com/questions/338317/what-is-makeinfo-and-how-do-i-get-it)
- [LFS 11.2(Linux From Scratch)构建过程全记录(二):磁盘分区](https://www.cnblogs.com/alphainf/p/16663371.html)