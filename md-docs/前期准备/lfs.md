
# Linux from scracth

## 前言

本文用于记录 [Linux from scracth](https://www.linuxfromscratch.org/lfs/): 构建自己的 Linux 发行版 的踩坑记录

当前版本的 lfs11.3, 可以在 [downloads/stable](https://www.linuxfromscratch.org/lfs/downloads/stable/) 中找到英文资料, 笔者使用的是中文翻译版

如果你想问有什么意义, 那我只能说 just for fun, just for free. 另外就是我希望构建一个用于 gem5 fs的disk

由于存在软件版本的变动, 所以当前记录可能并不完全适用于读者的最新版本, 那我们废话不多说直接开始吧

## 准备

笔者使用的是 Windows11 操作系统, 配合 VMware workstation 17.0, [Ubuntu22.04 Desktop image(LTS)](https://releases.ubuntu.com/jammy/ubuntu-22.04.2-desktop-amd64.iso) 作为宿主系统, 构建过程省略

```bash
kamilu@kamilu:~$ uname -a
Linux kamilu 5.19.0-41-generic #42~22.04.1-Ubuntu SMP PREEMPT_DYNAMIC Tue Apr 18 17:40:00 UTC 2 x86_64 x86_64 x86_64 GNU/Linux
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

执行脚本, 可以看到整个系统是刚刚装上的, 新的很,基本都是 command not found

```bash
kamilu@kamilu:~$ bash version-check.sh
bash, version 5.1.16(1)-release
/bin/sh -> /usr/bin/dash
ERROR: /bin/sh does not point to bash
Binutils: version-check.sh: line 9: ld: command not found
version-check.sh: line 10: bison: command not found
yacc not found
Coreutils:  8.32
diff (GNU diffutils) 3.8
find (GNU findutils) 4.8.0
version-check.sh: line 21: gawk: command not found
/usr/bin/awk -> /usr/bin/mawk
version-check.sh: line 29: gcc: command not found
version-check.sh: line 30: g++: command not found
grep (GNU grep) 3.7
gzip 1.10
Linux version 5.19.0-41-generic (buildd@lcy02-amd64-045) (x86_64-linux-gnu-gcc (Ubuntu 11.3.0-1ubuntu1~22.04.1) 11.3.0, GNU ld (GNU Binutils for Ubuntu) 2.38) #42~22.04.1-Ubuntu SMP PREEMPT_DYNAMIC Tue Apr 18 17:40:00 UTC 2
version-check.sh: line 34: m4: command not found
version-check.sh: line 35: make: command not found
GNU patch 2.7.6
Perl version='5.34.0';
Python 3.10.4
sed (GNU sed) 4.8
tar (GNU tar) 1.34
version-check.sh: line 41: makeinfo: command not found
xz (XZ Utils) 5.2.5
version-check.sh: line 43: g++: command not found
g++ compilation failed
```