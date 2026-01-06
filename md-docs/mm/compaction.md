
# memory compaction

> 本文摘自 [Linux中的Memory Compaction [一]](https://zhuanlan.zhihu.com/p/81983973), 大佬写的很完善了, 这里复制记录一份

Linux使用的是虚拟地址,以提供进程地址空间的隔离。它还带来另一个好处,就是像vmalloc()这种分配,不用太在乎实际使用的物理内存的分布是否连续,因而也就弱化了物理内存才会面临的「内存碎片」问题。

但如果使用kmalloc()申请的内存,则要求物理内存必须是连续的。在系统中空闲内存的总量(比如空闲10个pages)大于申请的内存大小,但没有连续的物理内存时,我们可以通过 **migrate** (迁移/移动)空闲的page frame,来聚合形成满足需求的连续的物理内存

## 实现原理

来看下具体是如何操作的。假设现在有如下图所示的一段内存,其中有8个已使用的movable pages,8个空闲的free pages.

![20230822232634](https://raw.githubusercontent.com/learner-lu/picbed/master/20230822232634.png)

现在我们把左侧的4个movable pages和右侧的4个free pages交换位置,那么就会形成8个free pages全在左侧,8个movable pages全在右侧的情形。这时如果要分配order为3的内存就不是什么问题了

![20230822232652](https://raw.githubusercontent.com/learner-lu/picbed/master/20230822232652.png)

这样的一套机制在Linux中称为memory compaction, "compaction"中文直译过来的意思是「压实」,这里free pages就像是气泡一样,migration的过程相当于把这些气泡挤压了出来,把真正有内容的pages压实了。同时,这也可以看做是一个归纳整理的过程,因此"memory compaction"也被译做「内存规整」

在此过程中,需要两个**page scanners**,一个是"migration scanner",用于从source端寻找可移动的页面,然后将这些页面从它们所在的LRU链表中isolate出来;另一个是"free scanner",用于从target端寻找空闲的页面,然后将这些页面从buddy分配器中isolate出来

当一个page frame被migrate之后,使用这段内存的应用程序还怎么访问这个page的内容呢?这就又要归功于Linux中可以将物理实现与逻辑抽象相分离的虚拟内存机制了,程序访问的始终是虚拟地址,哪怕物理地址在migration的过程中被更改,只要修改下进程页表中对应的PTE项就可以了。虚拟地址保存不变,程序就"感觉"不到

由于要保持虚拟地址不变,像kernel space中线性映射的这部分物理内存就不能被migrate,因为线性映射中虚拟地址和物理地址之间是固定的偏移,migration导致的物理地址更改势必也造成虚拟地址的变化。也就是说,并非所有的page都是"可移动"的,这也是为什么buddy系统中划分了"migrate type".

## CMA 机制

在数据方向是从外设到内存的DMA传输中,如果外设本身不支持scatter-gather形式的DMA,或者CPU不具备使用虚拟地址作为目标地址的IOMMU/SMMU机制(通常较高端的芯片才具有),那么则要求DMA的目标地址必须在物理内存上是连续的。

传统的做法是在内核启动时,通过传递"mem="的内核参数,预留一部分内存,但这种预留的内存只能作为DMA传输的"私藏",即便之后DMA并没有真正使用这部分内存,也不能挪作他用,造成浪费

## 参考

- [Linux中的Memory Compaction [一]](https://zhuanlan.zhihu.com/p/81983973)
- [Linux中的Memory Compaction [二] - CMA](https://zhuanlan.zhihu.com/p/105745299)