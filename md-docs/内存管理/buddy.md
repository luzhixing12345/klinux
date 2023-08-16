
# buddy

Linux的Buddy内存分配算法是一种用于管理物理内存的分配算法,旨在解决内存分配和释放的碎片问题, 该算法被广泛应用于Linux操作系统中. 内存中有一些是被内核代码占用,还有一些是被特殊用途所保留,那么剩余的空闲内存都会交给内核内存管理系统来进行统一管理和分配

linux早期版本(比如0.11)管理的方式比较简单粗暴,直接用bitmap的思路标记物理页是否被使用,这样做带来最直接的问题:内存碎片化, 而且bitmap也不能直观的快速寻址连续空闲的内存,每次都要从头开始遍历查找

在内核分配内存时,必须记录页帧的已分配或空闲状态,以免两个进程使用同样的内存区域.由于内存分配和释放非常频繁,内核还必须保证相关操作尽快完成.**内核可以只分配完整的页帧.将内存划分为更小的部分的工作,则委托给用户空间中的标准库.标准库将来源于内核的页帧拆分为小的区域,并为进程分配内存**.

内核中很多时候要求分配连续页.为快速检测内存中的连续区域,内核采用了一种古老而历经检验的技术: 伙伴系统(Buddy system)

系统中的空闲内存块总是两两分组,每组中的两个内存块称作伙伴.伙伴的分配可以是彼此独立的.但如果两个伙伴都是空闲的,内核会将其合并为一个更大的内存块,作为下一层次上某个内存块的伙伴. 众所周知进程的内存分配是以虚拟页 4KB 为最小单位分配的(即使会出现内部碎片)

由于虚拟内存和页表转换的设计, 使得在虚拟内存中需要连续的页面在物理内存中不必连续, 但是我们仍然希望可以快速的

linux 中 buddy 的最大数量为定义在 include/linux/mmzone.h 中的 MAX_ORDER 决定, 默认值为 11, 也就是把所有的空闲页框分组为11个块链表,每个块链表分别包含大小为1,2,4,8,16,32,64,128,256,512和1024个连续页框的页框块.最大可以申请1024个连续页框,对应4MB大小的连续内存,每个页框块的第一个页框的物理地址是该块大小的整数倍

![2052730-20211217181654472-798857625](https://raw.githubusercontent.com/learner-lu/picbed/master/2052730-20211217181654472-798857625.png)

```c
/* Free memory management - zoned buddy allocator.  */
#ifndef CONFIG_ARCH_FORCE_MAX_ORDER
#define MAX_ORDER 11
#endif
#define MAX_ORDER_NR_PAGES (1 << (MAX_ORDER - 1))
```

下图展示一个 4 级的 buddy, 的分配和释放过程. 其中最小的 buddy 块为 4KB

![20230816164048](https://raw.githubusercontent.com/learner-lu/picbed/master/20230816164048.png)

在应用程序释放内存时,内核可以直接检查地址,来判断是否能够创建一组伙伴,并合并为一个更大的内存块放回到伙伴列表中,这刚好是内存块分裂的逆过程.这提高了较大内存块可用的可能性

```c
struct zone {
    /* free areas of different sizes */
    struct free_area free_area[MAX_ORDER];
}

struct free_area {
    struct list_head free_list[MIGRATE_TYPES];
    unsigned long nr_free;
};
```

数组 free_area 每个索引 i 存放的就是大小为 2^i 页面数量的分组,这样可以提升Buddy System分配内存时查找分组的效率.

free_area 中的空闲分组按照 MIGRATE_TYPES 进行了分组,每种类型都保存在一个双向链表中

分配内存时还需要很多标记类参数,例如指定目标 zone (是从 DMA 请求内存还是 NORMAL)、分配到的内存是否要全部置零、如果内存不足,是否可以触发内存回收机制等等,这些标记都定义在 include/linux/gfp.h 中

```c
// include/linux/gfp.h
// include/linux/gfp_types.h

#define ___GFP_DMA      0x01u
#define ___GFP_HIGHMEM      0x02u
#define ___GFP_DMA32        0x04u
#define ___GFP_MOVABLE      0x08u
#define ___GFP_RECLAIMABLE  0x10u
#define ___GFP_HIGH     0x20u
#define ___GFP_IO       0x40u
#define ___GFP_FS       0x80u
#define ___GFP_ZERO     0x100u
#define ___GFP_ATOMIC       0x200u

/* 通过位运算将不同的标记组合起来 */
#define GFP_ATOMIC  (__GFP_HIGH|__GFP_ATOMIC|__GFP_KSWAPD_RECLAIM)
#define GFP_KERNEL  (__GFP_RECLAIM | __GFP_IO | __GFP_FS)
#define GFP_KERNEL_ACCOUNT (GFP_KERNEL | __GFP_ACCOUNT)
#define GFP_NOWAIT  (__GFP_KSWAPD_RECLAIM)
```

> "MIGRATE_MOVABLE"表示page frame可移动,"MIGRATE_UNMOVABLE"与之相反,而"MIGRATE_CMA"则是专用于CMA区域, 分配的时候会首先按照请求的migrate type从对应的pageblocks中寻找,如果不成功,其他migrate type的pageblocks中也会考虑(类似于[Linux内存调节之lowmem reserve](https://zhuanlan.zhihu.com/p/81961211)中zone之间的fallback),在一定条件下,甚至有可能改变pageblock的migrate type.



本质上讲,buddy算法相对于原始的bitmap,优势在于:

1. 按照不同页框个数2^n重新组织了内存,便于快速查找用户所需大小的内存块(free_area数组的寻址时间复杂度是O(1))
2. 不停的分配、重组页框,在一定程度上减少了页框碎片

## 思考的问题

1. 9KB?
2. 大于 4MB

## 参考

- [深入浅出内存管理-- 伙伴系统(buddy system)](https://blog.csdn.net/rikeyone/article/details/85054231)
- [内存分配[二] - Buddy系统的原理](https://zhuanlan.zhihu.com/p/73562347)
- [内存分配[三] - Linux中Buddy系统的实现](https://zhuanlan.zhihu.com/p/105589621)
- [深入Linux内核架构.pdf](https://awesome-programming-books.github.io/linux/%E6%B7%B1%E5%85%A5Linux%E5%86%85%E6%A0%B8%E6%9E%B6%E6%9E%84.pdf)
- [Buddy System(伙伴系统)](https://s3.shizhz.me/linux-mm/3.2-wu-li-nei-cun/3.2.4-buddy-system-huo-ban-xi-tong)
- [linux源码解读(九):内存管理_buddy和slab](https://www.cnblogs.com/theseventhson/p/15703182.html)
- [Buddy memory allocation](https://en.wikipedia.org/wiki/Buddy_memory_allocation)