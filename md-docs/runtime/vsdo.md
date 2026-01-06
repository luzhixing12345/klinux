
# vsdo

系统调用(system call) 是 Linux 内核中的特殊例程, 用户空间应用程序要求执行特权任务, 例如读取或写入文件/打开套接字等。我们知道在 Linux 内核中调用系统调用是一项代价高昂的操作, 因为处理器必须中断当前正在执行的任务并将上下文切换到内核模式,然后在系统调用处理程序完成其工作后再次跳转到用户空间。

但是我们注意到, 对于系统调用 gettimeofday, 实际上并不涉及任何数据安全问题,因为特权用户root和非特权用户都会获得相同的结果。这就意味着其实完全可以通过新的实现机制来避免执行系统调用的开销,因为本质上gettimeofday()就是从内核中读取与时间相关的数据(虽然会有一些复杂的计算过程).与其费尽心力一定要通过陷入内核的方式来读取这些数据,不如在内核与用户态之间建立一段共享内存区域,由内核定期"推送"最新值到该共享内存区域,然后用户态程序在调用这些系统调用的glibc库函数的时候,**库函数并不真正执行系统调用,而是通过vsyscall page来读取该数据的最新值,相当于将系统调用改造成了函数调用,直接提升了执行性能**

因此为了加速某些系统调用, 我们来介绍一下 `vsyscall` 和 `vDSO`

## vsyscall

vsyscall or virtual system call 是 Linux 内核中第一个也是最古老的机制, 也是一个**过时的概念**, 这里简单介绍了解一下

vsyscall 背后的想法很简单。某些系统调用只是将数据返回到用户空间。如果内核将这些系统调用实现和相关数据映射到用户空间页面, 应用程序就可以像简单的函数调用一样触发这些系统调用。用户空间和内核空间之间不会有上下文切换

> 在 Linux 内核文档中找到有关此内存空间的信息x86_64 [linux mm.txt#L23](https://github.com/torvalds/linux/blob/16f73eb02d7e1765ccab3d2018e0bd98eb93d973/Documentation/x86/x86_64/mm.txt#L23)
>
> ```txt
> ffffffffff600000 - ffffffffffdfffff (=8 MB) vsyscalls
> ```

call chain:

```bash
start_kernel        # init/main.c
setup_arch          # arch/x86/kernel/setup.c
map_vsyscall        # arch/x86/entry/vsyscall/vsyscall_64.c
```

```c
void __init map_vsyscall(void)
{
    // 获取 vsyscall 页面的物理地址, .S 中定义的 __vsyscall_page 地址
	extern char __vsyscall_page;
	unsigned long physaddr_vsyscall = __pa_symbol(&__vsyscall_page);

	if (vsyscall_mode == EMULATE) {
		__set_fixmap(VSYSCALL_PAGE, physaddr_vsyscall,
			     PAGE_KERNEL_VVAR);
		set_vsyscall_pgtable_user_bits(swapper_pg_dir);
	}

    // config 默认是走 XONLY 这个分支
	if (vsyscall_mode == XONLY)
		vm_flags_init(&gate_vma, VM_EXEC);

	BUILD_BUG_ON((unsigned long)__fix_to_virt(VSYSCALL_PAGE) !=
		     (unsigned long)VSYSCALL_ADDR);
}
```

初始化的过程位于函数 `vsyscall_setup`, 其将在早期内核参数解析期间调用, 有关 early_param 宏的更多信息见 "内核初始化" 部分

```c
early_param("vsyscall", vsyscall_setup);
```

由于所有的进程都共享内核映射,因此所有的进程也自然而言能够访问到vsyscall page, `__kernel_vsyscall` 是一个特殊的页,其位于内核地址空间,但也是唯一允许用户访问的区域

[arch/x86/entry/vsyscall/vsyscall_emu_64.S](https://github.com/torvalds/linux/blob/16f73eb02d7e1765ccab3d2018e0bd98eb93d973/arch/x86/entry/vsyscall/vsyscall_emu_64.S) 可以看到包含以下三个系统调用的调用 gettimeofday time getcpu

但是后来开发人员又抛弃了vsyscall机制, 原因有两点:

1. 映射地址在每个进程中都是**固定的**.固定地址被认为违反了 ASLR (地址空间布局随机化), 这使得攻击更容易写入漏洞, 因此很容易成为ret2libc攻击的跳板。
2. vsyscall能支持的系统调用数有限,无法很方便地进行扩展

> MIT-6.S081 的 lab3 的第一个实验就是加速系统调用

## VDSO

`vDSO` 是 `virtual dynamic shared object` 的缩写,表示这段mapping实际包含的是一个ELF共享目标文件,也就是俗称的。so

通常来说,大多数情况下我们不需要关系有关vDSO的实现细节,因为只有对它的调用是由C库完成的,所以vDSO对大多数人来说都是透明的。如果尝试在自己的应用程序中调用 vDSO,而不是使用 C 库,则很可能做错了

源码位于: arch/x86/entry/vdso/

### 快慢系统调用

在x86-32系统大行其道的时代,调用系统调用的方法就是 `int $0x80`.这种方法的执行速度非常慢,原因是它**需要经历一个完整的中断处理过程**,这包括Linux内核以及与中断流程相关的处理器微码的执行开销。

后来为了提升系统调用的性能,Intel最先实现了专门的快速系统调用指令 `sysenter` 和系统调用返回指令 `sysexit` ;后来AMD针锋相对地实现了另一组专门的快速系统调用指令 `syscall` 和系统调用返回指令 `sysret`.

快速系统调用的"快"字,体现在以下几个方面:

- 处理器在切换到内核态后不再自动往内核栈中自动保存任何上下文信息了,这样避免了访内开销。
- 处理器也不再自动加载内核栈的值到rsp寄存器了,节省了指令开销。
- syscall和sysret指令只能用在平坦内存模型中,因此在执行快速系统调用时bypass了MMU的分段单元的检查,节省了微码的执行开销。
- 处理器微码不再需要走中断处理和中断恢复流程,大幅度提高了执行性能。

与Intel的快速系统调用指令相比,AMD的syscall/sysret要更快更灵活:

- 执行syscall和sysret指令时,不再需要处理器自动保存和恢复用户栈指针了,因此也不需要再事先设置MSR来指定要恢复和保存的用户栈指针了。
- 通过系统调用进入内核态后,rflags寄存器中哪些位应该清0原本是固定的,但是如果是用syscall来执行系统调用的话,那这些位是可以通过编程来事先设置的。

最后,Intel也提供了对sysenter/sysexit指令的支持。之后,为了获得最好的兼容性(Intel和AMD通用),x86-64 **Linux内核将快速系统调用的支持方式统一到了syscall/sysret.**

### 实现细节

我们可以使用 ldd 来查看一个可执行文件的动态链接库依赖

```bash
(base) kamilu@LZX:~$ ldd `which bash`
        linux-vdso.so.1 (0x00007fffd2bec000)
        libtinfo.so.6 => /lib/x86_64-linux-gnu/libtinfo.so.6 (0x00007f31064cc000)
        libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f31062a4000)
        /lib64/ld-linux-x86-64.so.2 (0x00007f3106678000)
```

> 多次执行可以发现每一次的 .so 地址都不固定, 这就是利用 ASLR 防止攻击者可靠地跳转到内存中被利用的特定函数,ASLR 会随机排列进程关键数据区域的地址空间位置,包括可执行文件的基础以及堆栈/堆和库的位置

当内核加载一个ELF可执行程序时,内核都会在其进程地址空间中建立一个叫做vDSO mapping的内存区域。

vDSO与vsyscall最大的不同体现在以下方面:

- vDSO本质上是一个**ELF共享目标文件**;而vsyscall只是**一段内存代码和数据**, 映射到一个固定的地址区间。
- vsyscall位于内核地址空间,采用**静态地址映射方式**; 而vDSO借助共享目标文件天生具有的PIC特性,可以**以进程为粒度动态映射到进程地址空间**中

### 调用原理示例

我们以 `getcpu` 这个系统调用为例, 看一下 vdso 如何实现用户态调用

getcpu 系统调用被声明为 `__vdso_getcpu` 的一个弱符号, 相当于做了一个函数换名, 内部转而调用 `vdso_read_cpunode`

```c
// arch/x86/entry/vdso/vgetcpu.c
long __vdso_getcpu(unsigned *cpu, unsigned *node, struct getcpu_cache *unused)
{
	vdso_read_cpunode(cpu, node);

	return 0;
}

long getcpu(unsigned *cpu, unsigned *node, struct getcpu_cache *tcache)
	__attribute__((weak, alias("__vdso_getcpu")));
```

而 vdso_read_cpunode 内部则使用汇编从 GDT 中读取对应的 cpu 和 NUMA 节点, 整个过程没有陷入内核态

```c
// arch/x86/include/asm/segment.h
#define alternative_io(oldinstr, newinstr, ft_flags, output, input...) \
    asm_inline volatile(ALTERNATIVE(oldinstr, newinstr, ft_flags)  \
                : output                                   \
                : "i"(0), ##input)

static inline void vdso_read_cpunode(unsigned *cpu, unsigned *node)
{
	unsigned int p;

	/*
	 * Load CPU and node number from the GDT.  LSL is faster than RDTSCP
	 * and works on all CPUs.  This is volatile so that it orders
	 * correctly with respect to barrier() and to keep GCC from cleverly
	 * hoisting it out of the calling function.
	 *
	 * If RDPID is available, use it.
	 */
	alternative_io ("lsl %[seg],%[p]",
			".byte 0xf3,0x0f,0xc7,0xf8", /* RDPID %eax/rax */
			X86_FEATURE_RDPID,
			[p] "=a" (p), [seg] "r" (__CPUNODE_SEG));

	if (cpu)
		*cpu = (p & VDSO_CPUNODE_MASK);
	if (node)
		*node = (p >> VDSO_CPUNODE_BITS);
}
```

### VDSO 的构建

vDSO image的构建过程比较复杂,下面是一个简要的过程描述:

- 编译出组成vDSO image的三个目标文件:vdso-note.o vclock_gettime.o vgetcpu.o 这几个目标文件主要就是实现了以下几个快速系统调用的函数实现:
  - clock_gettime()
  - gettimeofday()
  - time()
  - getcpu()
- 使用专门定义的外部链接脚本vdso.lds生成raw vDSO image文件vdso64.so.dbg
- 调用objdump将vdso64.so.dbg strip为vdso64.so.
- 使用vdso2c工具将vdso64.so转换为vdso-image-64.c.vdso-image-64.c定义的是有关vdso64.so的各种blob信息,其中最重要一部分就是将vdso64.so的全部内容以C字节数组的形式放到了vdso-image-64.c中。
- 将vdso-image-64.c编译为vdso-image-64.o,并与内核中vDSO初始化相关的代码一起编译进内核。

### libc 调用 vsdo

在内核加载ELF binary时,内核中的ELF loader会通过辅助向量来向用户态传递一些信息;而getauxval()就是用来通过辅助向量来获取这些信息的函数:

```c
#include <sys/auxv.h>

void *vdso = (uintptr_t) getauxval(AT_SYSINFO_EHDR);
```

辅助向量中的每个entry是一个键值对:key称作类型,value就是值。

与vDSO相关的辅助向量类型是 `AT_SYSINFO_EHDR`,该向量值保存了vDSO mapping的基地址。

当然,识别vDSO mapping只是第一步,伴随而来的是繁复的解析工作;这些工作通常由glibc来承担,应用程序只要还是按照传统方式直接调用C库即可

默认情况下,vDSO mapping总是开启;通过指定 `vdso=0` 来关闭vDSO mapping.

与vsyscall=native | emulate(默认值) | none选项共同配合,在定位与二进制兼容性相关的问题时,可以快速找出大体的问题方向。

## vvar

与vDSO mapping相伴的是一个叫vvar mapping的。vvar mapping的大小是8-12K,内容是vDSO mapping中的代码要访问的内核与用户进程之间要共享的数据。

进程启动时,vvar mapping的起始位置总是位于进程栈底之上的一个随机偏移(受ASLR实际配置的影响,比如randomize_va_space=1或2的情况);偏移值小于2M(PMD大小) - vvar mapping与vdso mapping的总和。

这样的算法就导致可以相对容易地根据进程的栈底的位置(该位置也是受ASLR的影响而随机创建的)/在最多2M地址空间内以4K为步长去probe.

## 性能测试

```c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>


int main(int argc, char **argv) {
    unsigned long i = 0;
    time_t (*f)(time_t *) = (time_t(*)(time_t *))0xffffffffff600400UL;

    if (!strcmp(argv[1], "vsyscall")) {
        for (i = 0; i < 1000000; ++i) f(NULL);
    } else if (!strcmp(argv[1], "vdso")) {
        for (i = 0; i < 1000000; ++i) time(NULL);
    } else {
        for (i = 0; i < 1000000; ++i) syscall(SYS_time, NULL);
    }

    return 0;
}
```

笔者当前的内核没有 vsyscall 页面的映射

```bash
(base) kamilu@LZX:~$ time ./a.out vsyscall
Segmentation fault

real    0m0.007s
user    0m0.000s
sys     0m0.007s
(base) kamilu@LZX:~$ time ./a.out vdso

real    0m0.002s
user    0m0.002s
sys     0m0.000s
(base) kamilu@LZX:~$ time ./a.out vsyscall-native

real    0m0.045s
user    0m0.035s
sys     0m0.009s
```

从测试结果来看, `vDSO` > `syscall` == `native vsyscall` > `emulated vsyscall`.其中:

- 上面的vsyscall的结果是emulate模式的,也是内核默认的设置,原因是native vsyscall存在安全缺陷
- native模式的性能与直接调用C函数几乎一致,因为原理上都是执行syscall指令
- vDSO兼具安全性和高性能

注意:可以通过内核命令行参数 `vsyscall=native|emulate(默认值)` 来设置vsyscall的运行模式

## 参考

### VSDO

- [man7 vdso.7](https://man7.org/linux/man-pages/man7/vdso.7.html)
- [Linux vDSO概述](https://zhuanlan.zhihu.com/p/436454953)
- [Linux内核特性之VDSO](https://blog.csdn.net/JuanA1/article/details/6904932)
- [System calls in the Linux kernel. Part 3.](https://0xax.gitbooks.io/linux-insides/content/SysCall/linux-syscall-3.html)
- [vsyscall-and-vdso](https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2019/02/13/vsyscall-and-vdso)

### ASLR

- [Address space layout randomization](https://en.wikipedia.org/wiki/Address_space_layout_randomization)
- [Kernel address space layout randomization](https://lwn.net/Articles/569635/)