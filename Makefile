#
# if you want the ram-disk device, define this to be the
# size in blocks.
#
RAMDISK = #-DRAMDISK=512

AS86	=as86 -0 -a # 8086汇编器 -0 生成8086目标程序
LD86	=ld86 -0    # 8086链接器 -a 生成gas gld部分兼容的代码

AS	=gas # GNU 汇编编译器
LD	=gld # 链接器

LDFLAGS	=-s -x -M
# -s 输出文件省略的所有符号信息
# -x 删除所有局部符号
# -M 需要在标准输出设备上打印链接映像

CC = gcc $(RAMDISK) # gcc 1.40
CFLAGS = -Wall -O -fstrength-reduce -fomit-frame-pointer \
-fcombine-regs -mstring-insns
# -Wall 打印所有警告信息
# -O 对代码进行优化
# -fstrength-reduce 优化循环语句
# -fomit-frame-pointer 生成目标代码时不要使用帧指针寄存器来保存当前函数的栈帧信息

# -fcombine-regs 将一些寄存器组合成一个更大的寄存器，从而减少指令的数目
# -mstring-insns 使用特殊的指令序列来处理字符串操作，以提高程序的执行效率
# 这两个选项适用于一些小型嵌入式系统和早期的x86架构中，但是在现代的x86架构中，处理器的寄存器数量已经足够多，因此该选项已经被默认禁用。


CPP	=cpp -nostdinc -Iinclude

# -nostdinc 不搜索标准头文件目录的文件
# -Iinclude 在include目录下搜索

#
# ROOT_DEV specifies the default root-device when making the image.
# This can be either FLOPPY, /dev/xxxx or empty, in which case the
# default of /dev/hd6 is used by 'build'.
#
ROOT_DEV=/dev/hd6

# kernel mm fs 所产生的目标文件 => ARCHIVES
ARCHIVES=kernel/kernel.o mm/mm.o fs/fs.o

# 块设备驱动 字符设备驱动 (静态库)=> DRIVERS
DRIVERS =kernel/blk_drv/blk_drv.a kernel/chr_drv/chr_drv.a

# 数学算数库
MATH	=kernel/math/math.a

# 通用库文件
LIBS	=lib/lib.a

# .c => .s
.c.s:
	$(CC) $(CFLAGS) \
	-nostdinc -Iinclude -S -o $*.s $<

# .s => .o
.s.o:
	$(AS) -c -o $*.o $<

# .c => .o
.c.o:
	$(CC) $(CFLAGS) \
	-nostdinc -Iinclude -c -o $*.o $<

all: Image

# 依赖四个文件 => boot/bootsect boot/setup tools/system tools/build
# 以 $(ROOT_DEV) 为根文件系统设备组装成内核映像文件 Image
Image: boot/bootsect boot/setup tools/system tools/build
	tools/build boot/bootsect boot/setup tools/system $(ROOT_DEV) > Image
	sync
# sync : 同步命令, 迫使缓冲块数据立即写盘并更新超级快

boot/bootsect:	boot/bootsect.s
	$(AS86) -o boot/bootsect.o $^
	$(LD86) -s -o $@ boot/bootsect.o
# -s 表示去除目标文件中的符号信息

boot/setup: boot/setup.s
	$(AS86) -o boot/setup.o $^
	$(LD86) -s -o $@ boot/setup.o

tools/system: boot/head.o init/main.o $(ARCHIVES) $(DRIVERS) $(MATH) $(LIBS)
	$(LD) $(LDFLAGS) $^ -o $@ > System.map

tools/build: tools/build.c
	$(CC) $(CFLAGS) -o $@ $^

# dd 为UNIX标准命令 复制一个文件, 根据选项进行转换和格式化
# bs: 一次读/写的字节数
# if: 输入的文件
# of: 输出的文件 => /dev/PS0 第一个软盘驱动器(设备文件)
disk: Image
	dd bs=8192 if=Image of=/dev/PS0

boot/head.o: boot/head.s

kernel/math/math.a:
	(cd kernel/math; make)

kernel/blk_drv/blk_drv.a:
	(cd kernel/blk_drv; make)

kernel/chr_drv/chr_drv.a:
	(cd kernel/chr_drv; make)

kernel/kernel.o:
	(cd kernel; make)

mm/mm.o:
	(cd mm; make)

fs/fs.o:
	(cd fs; make)

lib/lib.a:
	(cd lib; make)

# 在bootsect.s 程序开头添加一行有关system文件的长度信息
tmp.s:	boot/bootsect.s tools/system
	(echo -n "SYSSIZE = (";ls -l tools/system | grep system \
		| cut -c25-31 | tr '\012' ' '; echo "+ 15 ) / 16") > tmp.s
	cat boot/bootsect.s >> tmp.s

clean:
	rm -f Image System.map tmp_make core boot/bootsect boot/setup
	rm -f init/*.o tools/system tools/build boot/*.o
	(cd mm;make clean)
	(cd fs;make clean)
	(cd kernel;make clean)
	(cd lib;make clean)

# 压缩linux文件夹为  backup.Z
backup: clean
	(cd .. ; tar cf - linux | compress - > backup.Z)
	sync

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in init/*.c;do echo -n "init/";$(CPP) -M $$i;done) >> tmp_make
	cp tmp_make Makefile
	(cd fs; make dep)
	(cd kernel; make dep)
	(cd mm; make dep)

### Dependencies:
init/main.o : init/main.c include/unistd.h include/sys/stat.h \
  include/sys/types.h include/sys/times.h include/sys/utsname.h \
  include/utime.h include/time.h include/linux/tty.h include/termios.h \
  include/linux/sched.h include/linux/head.h include/linux/fs.h \
  include/linux/mm.h include/signal.h include/asm/system.h include/asm/io.h \
  include/stddef.h include/stdarg.h include/fcntl.h 
