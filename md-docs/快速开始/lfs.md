
# Linux from scracth

## 前言

本文用于记录 [Linux from scracth](https://www.linuxfromscratch.org/lfs/): 构建自己的 Linux 发行版 的踩坑记录

当前版本的 lfs11.3, 可以在 [downloads/stable](https://www.linuxfromscratch.org/lfs/downloads/stable/) 中找到英文资料, 笔者使用的是中文翻译版

常见问题见 [lfs FAQ](https://www.linuxfromscratch.org/faq/)

## 准备

笔者使用的是 Windows11 操作系统, 配合 WSL2 作为宿主系统, 关于 WSL2 的配置请参考 [WSL2配置](https://luzhixing12345.github.io/2022/10/06/环境配置/WSL2配置/)

```bash
(base) kamilu@LZX:~/cfs/lfs$ uname -a
Linux LZX 5.10.102.1-microsoft-standard-WSL2 #1 SMP Wed Mar 2 00:30:59 UTC 2022 x86_64 x86_64 x86_64 GNU/Linux
```

执行下面的命令得到一个 `version-check.sh` 版本检查脚本

```bash
cat > version-check.sh << "EOF"
#!/bin/bash
# Simple script to list version numbers of critical development tools
export LC_ALL=C
bash --version | head -n1 | cut -d" " -f2-4
MYSH=$(readlink -f /bin/sh)
echo "/bin/sh -> $MYSH"
echo $MYSH | grep -q bash || echo "ERROR: /bin/sh does not point to bash"
unset MYSH
echo -n "Binutils: "; ld --version | head -n1 | cut -d" " -f3-
bison --version | head -n1
if [ -h /usr/bin/yacc ]; then
 echo "/usr/bin/yacc -> `readlink -f /usr/bin/yacc`";
elif [ -x /usr/bin/yacc ]; then
 echo yacc is `/usr/bin/yacc --version | head -n1`
else
 echo "yacc not found"
fi
echo -n "Coreutils: "; chown --version | head -n1 | cut -d")" -f2
diff --version | head -n1
find --version | head -n1
gawk --version | head -n1
if [ -h /usr/bin/awk ]; then
 echo "/usr/bin/awk -> `readlink -f /usr/bin/awk`";
elif [ -x /usr/bin/awk ]; then
 echo awk is `/usr/bin/awk --version | head -n1`
else
 echo "awk not found"
fi
gcc --version | head -n1
g++ --version | head -n1
grep --version | head -n1
gzip --version | head -n1
cat /proc/version
m4 --version | head -n1
make --version | head -n1
patch --version | head -n1
echo Perl `perl -V:version`
python3 --version
sed --version | head -n1
tar --version | head -n1
makeinfo --version | head -n1 # texinfo version
xz --version | head -n1
echo 'int main(){}' > dummy.c && g++ -o dummy dummy.c
if [ -x dummy ]
 then echo "g++ compilation OK";
 else echo "g++ compilation failed"; fi
rm -f dummy.c dummy
EOF
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

> [stackexchange unix](https://unix.stackexchange.com/questions/442510/how-to-use-bash-for-sh-in-ubuntu)

makeinfo 的问题安装 textinfo 即可

```bash
sudo apt install texinfo
```

> [what is makeinfo and how do i get it](https://stackoverflow.com/questions/338317/what-is-makeinfo-and-how-do-i-get-it)

接下来执行没有任何问题, 笔者的软件版本号如下:

```bash
bash, version 5.1.16(1)-release
/bin/sh -> /usr/bin/bash
Binutils: (GNU Binutils for Ubuntu) 2.38
bison (GNU Bison) 3.8.2
/usr/bin/yacc -> /usr/bin/bison.yacc
Coreutils:  8.32
diff (GNU diffutils) 3.8
find (GNU findutils) 4.8.0
GNU Awk 5.1.0, API: 3.0 (GNU MPFR 4.1.0, GNU MP 6.2.1)
/usr/bin/awk -> /usr/bin/gawk
gcc (Ubuntu 9.5.0-1ubuntu1~22.04) 9.5.0
g++ (Ubuntu 11.3.0-1ubuntu1~22.04.1) 11.3.0
grep (GNU grep) 3.7
gzip 1.10
Linux version 5.10.102.1-microsoft-standard-WSL2 (oe-user@oe-host) (x86_64-msft-linux-gcc (GCC) 9.3.0, GNU ld (GNU Binutils) 2.34.0.20200220) #1 SMP Wed Mar 2 00:30:59 UTC 2022
m4 (GNU M4) 1.4.18
GNU Make 4.3
GNU patch 2.7.6
Perl version='5.34.0';
Python 3.9.13
sed (GNU sed) 4.8
tar (GNU tar) 1.34
texi2any (GNU texinfo) 6.8
xz (XZ Utils) 5.2.6
g++ compilation OK
```