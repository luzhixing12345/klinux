
# NUMA

> 阅读本文需要掌握 [多处理器](../arch/multicore.md)的相关知识

## CPU 术语

大多数CPU都是在二维平面上构建的.CPU还必须添加集成内存控制器.对于每个CPU核心,有四个内存总线(上,下,左,右)的简单解决方案允许完全可用的带宽,但仅此而已.CPU在很长一段时间内都停滞在4核状态.当芯片变成3D时,在上面和下面添加痕迹允许直接总线穿过对角线相反的CPU.在卡上放置一个四核CPU,然后连接到总线,这是合乎逻辑的下一步.

如今每个处理器都包含许多核心,这些核心都有一个共享的片上缓存和片外内存,并且在服务器内不同内存部分的内存访问成本是可变的. 提高数据访问效率是当前CPU设计的主要目标之一, 因此每个CPU核都被赋予了一个较小的一级缓存(32 KB)和一个较大的二级缓存(256 KB).各个核心随后共享几个MB的3级缓存,其大小随着时间的推移而大幅增长.

> 为了避免缓存丢失(请求不在缓存中的数据),需要花费大量的研究时间来寻找合适的CPU缓存数量,缓存结构和相应的算法. 详见 [缓存一致性](./cc.md)

一个 CPU 有如下的一些术语: `socket` `core` `ucore` `threads`

![20240510153146](https://raw.githubusercontent.com/learner-lu/picbed/master/20240510153146.png)

- Socket: **一个Socket对应一个物理CPU**. 这个词大概是从CPU在主板上的物理连接方式上来的,可以理解为 Socket 就是主板上的 CPU 插槽.处理器通过主板的Socket来插到主板上. 尤其是有了多核(Multi-core)系统以后,Multi-socket系统被用来指明系统到底存在多少个物理CPU.
- Core: **CPU的运算核心**. x86的核包含了CPU运算的基本部件,如逻辑运算单元(ALU), 浮点运算单元(FPU), L1和L2缓存. 一个Socket里可以有多个Core.如今的多核时代,即使是单个socket的系统, 由于每个socket也有多个core, 所以逻辑上也是SMP系统.

  > 但是,一个物理CPU的系统不存在非本地内存,因此相当于UMA系统.

- Uncore: Intel x86物理CPU里没有放在Core里的部件都被叫做Uncore.Uncore里集成了过去x86 UMA架构时代北桥芯片的基本功能. 在Nehalem时代,内存控制器被集成到CPU里,叫做iMC(Integrated Memory Controller). 而PCIe Root Complex还做为独立部件在IO Hub芯片里.到了SandyBridge时代,PCIe Root Complex也被集成到了CPU里. 现今的Uncore部分,除了iMC,PCIe Root Complex,还有QPI(QuickPath Interconnect)控制器, L3缓存,CBox(负责缓存一致性),及其它外设控制器.
- Threads: 这里特指CPU的多线程技术.在Intel x86架构下,CPU的多线程技术被称作超线程(Hyper-Threading)技术. Intel的超线程技术在一个处理器Core内部引入了额外的硬件设计模拟了两个逻辑处理器(Logical Processor), 每个逻辑处理器都有独立的处理器状态,但共享Core内部的计算资源,如ALU,FPU,L1,L2缓存. 这样在最小的硬件投入下提高了CPU在多线程软件工作负载下的性能,提高了硬件使用效率. x86的超线程技术出现早于NUMA架构.

因此, 一个CPU Socket里可以由多个CPU Core和一个Uncore部分组成.每个CPU Core内部又可以由两个CPU Thread组成. 每个CPU thread都是一个操作系统可见的逻辑CPU.对大多数操作系统来说,一个八核HT(Hyper-Threading)打开的CPU会被识别为16个CPU

如下图所示

![20240512092458](https://raw.githubusercontent.com/learner-lu/picbed/master/20240512092458.png)

> QPI(QuickPath Interconnect)是英特尔(Intel)处理器架构中使用的一种高速互联技术.它用于处理器与其他组件(如内存,I/O设备和其他处理器)之间的通信.
>
> LLC(Last-Level Cache):LLC 是处理器架构中的最后一级缓存.在多级缓存结构中,处理器通常具有多个级别的缓存,而最后一级缓存(通常是共享的)被称为 LLC.LLC 位于处理器核心和主存之间,用于存储频繁访问的数据,以加快处理器对数据的访问速度.

## NUMA系统

NUMA体系结构中多了Node的概念,这个概念其实是用来解决core的分组的问题.**每个node有自己的内部CPU,总线和内存**,同时还可以访问其他node内的内存,NUMA的最大的优势就是可以方便的增加CPU的数量.NUMA系统中,**内存的划分是根据物理内存模块和内存控制器的布局来确定的**

![image](https://raw.githubusercontent.com/learner-lu/picbed/master/20240224114939.png)

在Intel x86平台上:

- 本地内存指 CPU 可以经过Uncore部件里的iMC访问到的内存.
- 远程内存(Remote Memory),则需要经过QPI的链路到该内存所在的CPU的iMC来访问.

与本地内存一样,所谓本地IO资源,就是CPU可以经过Uncore部件里的PCIe Root Complex直接访问到的IO资源. 如果是非本地IO资源,则需要经过QPI链路到该IO资源所属的CPU,再通过该CPU PCIe Root Complex访问. 如果同一个NUMA Node内的CPU和内存和另外一个NUMA Node的IO资源发生互操作,因为要跨越QPI链路, 会存在额外的访问延迟问题

> 曾经在Intel IvyBridge的NUMA平台上做的内存访问性能测试显示,远程内存访问的延时时本地内存的一倍.

一个NUMA Node内部是由一个物理CPU和它所有的本地内存, 本地IO资源组成的. 通常一个 Socket 有一个 Node,也有可能一个 Socket 有多个 Node.

```bash
root@kamilu:~$ sudo apt install numactl

root@kamilu:~$ numactl -H
available: 3 nodes (0-2)
node 0 cpus: 0 1 2 3 4 5 6 7 8 9 10 11 24 25 26 27 28 29 30 31 32 33 34 35
node 0 size: 31819 MB
node 0 free: 10283 MB
node 1 cpus: 12 13 14 15 16 17 18 19 20 21 22 23 36 37 38 39 40 41 42 43 44 45 46 47
node 1 size: 32193 MB
node 1 free: 26965 MB
node 2 cpus:
node 2 size: 16384 MB
node 2 free: 16375 MB
node distances:
node   0   1   2
  0:  10  21  24
  1:  21  10  14
  2:  24  14  10
```

### NUMA互联

在Intel x86上,NUMA Node之间的互联是通过 QPI(QuickPath Interconnect) Link的. CPU的Uncore部分有QPI的控制器来控制CPU到QPI的数据访问, 下图就是一个利用 QPI Switch 互联的 8 NUMA Node 的 x86 系统

![numa-imc-iio-qpi-switch-3](https://raw.githubusercontent.com/learner-lu/picbed/master/numa-imc-iio-qpi-switch-3.png)

### NUMA亲和性

NUMA Affinity(亲和性)是和NUMA Hierarchy(层级结构)直接相关的.对系统软件来说, 以下两个概念至关重要

- CPU NUMA Affinity

  CPU NUMA的亲和性是指从CPU角度看,哪些内存访问更快,有更低的延迟.如前所述, 和该CPU直接相连的本地内存是更快的.操作系统如果可以根据任务所在CPU去分配本地内存, 就是基于CPU NUMA亲和性的考虑.因此,CPU NUMA亲和性就是要**尽量让任务运行在本地的NUMA Node里.**

- Device NUMA Affinity

  设备NUMA亲和性是指从PCIe外设的角度看,如果和CPU和内存相关的IO活动都发生在外设所属的NUMA Node, 将会有更低延迟.这里有两种设备NUMA亲和性的问题

  1. DMA Buffer NUMA Affinity

     大部分PCIe设备支持DMA功能的.也就是说,设备可以直接把数据写入到位于内存中的DMA缓冲区. 显然,如果DMA缓冲区在PCIe外设所属的NUMA Node里分配,那么将会有最低的延迟. 否则,外设的DMA操作要跨越QPI链接去读写另外一个NUMA Node里的DMA缓冲区. 因此,**操作系统如果可以根据PCIe设备所属的NUMA node分配DMA缓冲区**, 将会有最好的DMA操作的性能.

  2. Interrupt NUMA Affinity

     当设备完成DMA操作后,它会发送一个中断信号给CPU,通知CPU需要处理相关的中断处理例程(ISR),这个例程负责读写DMA缓冲区的数据. ISR在某些情况下会触发下半部机制(SoftIRQ),以便进入与协议栈(如网络,存储)相关的代码路径,以传输数据. 对大部分操作系统来说,硬件中断(HardIRQ)和下半部机制的代码在同一个CPU上发生. 因此,**如果操作系统能够将设备的硬件中断绑定到与操作系统自身所属的NUMA节点相对应的处理器上**,那么中断处理函数和协议栈代码对DMA缓冲区的读写操作将会具有更低的延迟.这样做可以减少处理器间的通信延迟,提高系统性能.

> 中断处理例程(Interrupt Service Routine, ISR)用于响应硬件中断事件,尽快地处理中断并进行必要的操作. 当一个设备或外部事件触发了一个硬件中断,CPU会中断当前执行的任务,并跳转到相应的ISR来处理中断
>
> 半部机制(SoftIRQ)是一种延迟处理机制,用于处理与中断相关的一些非关键,耗时较长的任务. SoftIRQ不是由硬件中断触发,而是在上下文切换,网络数据包处理,定时器等事件发生时,由内核调度执行的

### Firmware接口

由于NUMA的亲和性对应用的性能非常重要,那么硬件平台就需要给操作系统提供接口机制来感知硬件的NUMA层级结构. 在x86平台,ACPI规范提供了以下接口来让操作系统来检测系统的NUMA层级结构.

> ACPI(Advanced Configuration and Power Interface)是一种开放标准,旨在为操作系统和硬件之间提供统一的接口,以实现高级配置和电源管理功能, 包括ACPI表, 系统电源管理, 系统配置和资源管理, 事件处理, 系统配置表和命名空间. ACPI规范的广泛应用使得不同的操作系统和硬件厂商能够以一致的方式进行交互,提高了系统的兼容性和可移植性
>
> ACPI 5.0a规范的第17章是有关NUMA的章节.ACPI规范里,NUMA Node被第9章定义的Module Device所描述. **ACPI规范里用Proximity Domain(接近性域)对NUMA Node做了抽象,两者的概念大多时候等同.**

- SRAT(System Resource Affinity Table) 系统资源关联表, 用于优化资源分配和调度

  主要描述了系统boot时的CPU和内存都属于哪个Proximity Domain(NUMA Node). 这个表格里的信息时静态的,
  
  > 如果是启动后热插拔,需要用OSPM的_PXM方法去获得相关信息.

- SLIT(System Locality Information Table) 系统局部性信息表, 用于优化任务调度和资源分配

  **提供CPU和内存之间的位置远近信息**.在SRAT表格里,只能告诉给定的CPU和内存是否在一个NUMA Node. 对某个CPU来说,不在本NUMA Node里的内存,即远程内存们是否都是一样的访问延迟取决于NUMA的拓扑有多复杂(QPI的跳数). 总之,**对于不能简单用远近来描述的NUMA系统**(QPI存在0,1,2等不同跳数), 需要SLIT表格给出进一步的说明.
  
  > 也是静态表格,热插拔需要使用OSPM的_SLI方法.

- DSDT(Differentiated System Description Table) 不同化系统描述表, 用于正确识别和管理硬件和设备

  从 Device NUMA角度看,这个表格给出了系统boot时的外设都属于哪个Proximity Domain(NUMA Node).

> 关于 ACPI 的一些介绍见本系列的 arch/ACPI

## node 初始化

node的初始化,从几条启动日志开始说起

![20230705155010](https://raw.githubusercontent.com/learner-lu/picbed/master/20230705155010.png)

通过第一条日志顺藤摸瓜可以找到如下的函数

```c
/**
 * dummy_numa_init - Fallback dummy NUMA init
 *
 * Used if there's no underlying NUMA architecture, NUMA initialization
 * fails, or NUMA is disabled on the command line.
 *
 * Must online at least one node and add memory blocks that cover all
 * allowed memory.  This function must not fail.
 */
// arch/x86/mm/numa.c
static int __init dummy_numa_init(void)
{
	printk(KERN_INFO "%s\n",
	       numa_off ? "NUMA turned off" : "No NUMA configuration found");
    /* max_pfn是e820探测到的最大物理内存页,其初始化是max_pfn = e820__end_of_ram_pfn() */
	printk(KERN_INFO "Faking a node at [mem %#018Lx-%#018Lx]\n",
	       0LLU, PFN_PHYS(max_pfn) - 1);
    /* 一个nodemask_t是 位图, 最多支持MAX_NUMNODES个node
     * 这里将node 0置位
     */
	node_set(0, numa_nodes_parsed);
    /* 将node 0的起始和结束地址记录起来 */
	numa_add_memblk(0, 0, PFN_PHYS(max_pfn));

	return 0;
}

int __init numa_add_memblk(int nid, u64 start, u64 end)
{
	return numa_add_memblk_to(nid, start, end, &numa_meminfo);
}

// arch/x86/mm/numa_internal.h
struct numa_meminfo {
	int			nr_blks;
	struct numa_memblk	blk[NR_NODE_MEMBLKS];
};

struct numa_memblk {
	u64			start;
	u64			end;
	int			nid;
};
```

代码和注释写的很清晰, 当没有 NUMA 架构或者 NUMA 架构被禁止的时候, Linux为了适配两者,将UMA"假装"(fake)成一种NUMA架构,也就只有一个node 0节点,该节点包括所有物理内存. numa_nodes_parsed 为 NUMA 节点的位图, 每一个bit代表一个node,node_set是将一个node设置为"在线".

numa_add_memblk 就是将一个 node 加入到 numa_meminfo, 并设置内存地址的范围. numa_meminfo, numa_memblk 的结构体也比较清晰

> PFN_PHYS(max_pfn) 宏用于获取探测到的最大物理内存页的范围

numa_add_memblk_to 逻辑也比较简单, 就是先做一些配置上的判断, 然后结构体对应元素赋值

```c
static int __init numa_add_memblk_to(int nid, u64 start, u64 end,
				     struct numa_meminfo *mi)
{
	/* ignore zero length blks */
	if (start == end)
		return 0;

	/* whine about and ignore invalid blks */
	if (start > end || nid < 0 || nid >= MAX_NUMNODES) {
		pr_warn("Warning: invalid memblk node %d [mem %#010Lx-%#010Lx]\n",
			nid, start, end - 1);
		return 0;
	}

	if (mi->nr_blks >= NR_NODE_MEMBLKS) {
		pr_err("too many memblk ranges\n");
		return -EINVAL;
	}

	mi->blk[mi->nr_blks].start = start;
	mi->blk[mi->nr_blks].end = end;
	mi->blk[mi->nr_blks].nid = nid;
	mi->nr_blks++;
	return 0;
}
```

整个系统的函数调用栈如下, numa_init 中也可以看出, 无论是否有 NUMA, 区别只是对所传递的函数指针的调用的差别而已

```c
// arch/x86/kernel/setup.c | setup_arch
// arch/x86/mm/numa_64.c   | initmem_init
// arch/x86/mm/numa.c      | x86_numa_init
// arch/x86/mm/numa.c      | numa_init

void __init x86_numa_init(void)
{
	if (!numa_off) {
#ifdef CONFIG_ACPI_NUMA
		if (!numa_init(x86_acpi_numa_init))
			return;
#endif
#ifdef CONFIG_AMD_NUMA
		if (!numa_init(amd_numa_init))
			return;
#endif
	}
	numa_init(dummy_numa_init);
}
```

## 参考

- [深入理解Linux内存管理 专栏](https://www.zhihu.com/column/c_1444822980567805952)
- [Linux内存管理 专栏](https://www.zhihu.com/column/c_1543333099974721536)
- [zhou-yuxin's blog](https://zhou-yuxin.github.io/)
- [内存管理相关数据结构之pg_data_t](https://www.cnblogs.com/linhaostudy/p/12679441.html)
- [articles](https://zhou-yuxin.github.io/articles/2018/Linux%E7%89%A9%E7%90%86%E5%86%85%E5%AD%98%E7%AE%A1%E7%90%86%E2%80%94%E2%80%94%E8%8E%B7%E5%8F%96%E7%89%A9%E7%90%86%E5%86%85%E5%AD%98%E5%B8%83%E5%B1%80%E3%80%81%E5%88%92%E5%88%86%E5%86%85%E5%AD%98%E5%8C%BA%E4%B8%8E%E5%88%9B%E5%BB%BANUMA%E8%8A%82%E7%82%B9/index.html)
- [Linux物理内存管理_获取物理内存布局,划分内存区与创建NUMA节点](https://zhou-yuxin.github.io/articles/2018/Linux物理内存管理_获取物理内存布局,划分内存区与创建NUMA节点/index.html)
- [Performance Analysis of UMA and NUMA Models](https://citeseerx.ist.psu.edu/viewdoc/download;jsessionid=09667FE3B088F3927E29EF7518DDA56F?doi=10.1.1.414.3607&rep=rep1&type=pdf)
- [理解 NUMA 架构](https://zhuanlan.zhihu.com/p/534989692)
- [linuxhint understanding_numa_architecture](https://linuxhint.com/understanding_numa_architecture/)
- [Linux 内核 101:NUMA架构](https://zhuanlan.zhihu.com/p/62795773)
- [十年后数据库还是不敢拥抱NUMA](https://plantegg.github.io/2021/05/14/十年后数据库还是不敢拥抱NUMA/): 好文
- [[计算机体系结构]NUMA架构详解](https://houmin.cc/posts/b893097a/): 好文
- [NUMA背后的设计思想](https://frankdenneman.nl/2016/07/07/numa-deep-dive-part-1-uma-numa/)
- [Bus snooping](https://en.wikipedia.org/wiki/Bus_snooping)
- [cache-coherence-protocols-in-multiprocessor-system](https://www.geeksforgeeks.org/cache-coherence-protocols-in-multiprocessor-system/)
- [NUMA与UEFI](https://zhuanlan.zhihu.com/p/26078552)