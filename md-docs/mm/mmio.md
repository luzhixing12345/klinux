
# mmio

开始之前我们先思考一个问题: IO 设备如何映射到运行 Linux 的现代 x86 机器上的"常规"内存地址空间?

计算机上电之后, 从操作系统的视角来看它管理了一大片连续的物理内存. 但是除了 CPU 和内存之外, 还有其他 IO 设备也有可能会有自己的内存, 比如一些 FPGA 设备, 一些内存扩展设备等等. 这些设备通过 PCIe 总线最终连接到 CPU, 那么操作系统是如何识别并管理这些设备的呢? 如何访问设备中的寄存器和内存, 设备又是如何访问系统主存的呢?

## 设备枚举

当计算机启动时,操作系统的内核会进行一个名为"设备枚举"的过程.这包括:

1. **扫描和识别设备**:
    - 内核扫描PCI总线,找到所有连接的设备.
    - 读取每个设备的配置空间,以获取设备类型和功能信息.

2. **读取和分配MMIO空间**:
    - 内核读取每个设备的BAR寄存器,获取设备的MMIO空间需求.
    - 内核在系统地址空间中为设备分配合适的地址范围,并将这些地址写入设备的BAR寄存器.

    > BAR 空间和 MMIO 下文会提到

3. **配置主机桥**:
    - 主机桥配置为正确路由来自CPU的内存和I/O请求到相应的设备.
    - 确保主机桥能处理设备的中断请求和DMA请求.

例如下图为 USB 设备接入之后的枚举过程:

![20240722014120](https://raw.githubusercontent.com/learner-lu/picbed/master/20240722014120.png)

> 完整过程相当复杂, 这里不做展开
>
> [USB设备的枚举过程分析](https://www.usbzh.com/article/detail-419.html)
> 
> [USB 枚举/断开过程](https://www.usbzh.com/article/detail-110.html)

假设一个系统中有一个PCI网卡,启动过程如下:
1. **内核扫描PCI总线**,找到网卡设备.
2. **读取网卡的配置空间**,获取其BAR寄存器,假设BAR0中存储的是网卡控制寄存器的基地址.
3. **内核为网卡分配一段内存空间**,比如 `0xD0000000 - 0xE0000000`,并将该地址写入BAR0.
4. **配置主机桥**,确保访问该地址范围的请求可以被正确路由到网卡设备.
5. **驱动程序使用映射的地址空间**(如0xD0000000)与网卡进行通信.

通过以上步骤,内核确保所有I/O设备的MMIO空间正确映射,并通过主机桥连接到系统,使CPU和设备能够正常交互.

在 [io](../arch/io.md) 的 IO 设备一节中我们提到, 现代管理访问 IO 设备操作普遍采用的是 **MMIO**,即Memory Mapped IO,也就是说把这些IO设备中的内部存储和寄存器都映射到统一的存储地址空间(Memory Address Space)中.

总线可以完成设备的注册和地址的转发. 在大多数系统中,分配给控制寄存器的地址位于或者靠近地址的顶部附近. 例如下图是笔者查看 `/proc/iomem` 的结果, 该文件用户记录物理地址映射范围.

![20240716171157](https://raw.githubusercontent.com/learner-lu/picbed/master/20240716171157.png)

可以看到在系统内存 System RAM 的 16GB 之外, 后面这几个 `e010a6b0` `711dad3a` `559c9870` 就是 IO 设备映射的物理地址范围.

当 CPU 想要读入一个字的时候,无论是从内存中读入还是从 I/O 端口读入,它都要将需要的地址放到总线地址线上,然后在总线的一条控制线上调用一个 `READ` 信号.还有第二条信号线来表明需要的是 **I/O 空间还是内存空间**.

- 如果位于内存空间, 内存将响应请求
- 否则将地址与每个 I/O 设备所服务的地址范围进行比较. 如果地址落在这一范围之内, 那么说明是该设备的 IO 空间. 发送给设备
- 否则认为该地址无效, 操作系统可以保证所有地址范围不会覆盖, 因此不会存在歧义和冲突

通过将外设的寄存器和内存映射到系统的地址空间的 MMIO 方案,使得CPU可以像访问普通内存一样访问这些外设.这种方法使得编程和访问更加统一和简便

## 地址空间

要彻底弄明白操作系统是如何管理设备地址空间, 就要了解多种类型的地址及其差异

- 内核通常使用**虚拟地址**. `kmalloc()` 、`vmalloc()` 和类似接口返回的任何地址都是虚拟地址, 表示为 `void *`
- 虚拟内存系统(TLB、页表等)将虚拟地址转换为 CPU **物理地址**,这些物理地址存储为 `phys_addr_t` 或 `resource_size_t`.
- I/O 设备使用第三种地址:"**总线地址**".

由于 MMU 的存在, **CPU 不能直接访问物理地址**, 而是需要将物理地址映射到虚拟地址才能访问. 对系统内存如此,对 I/O 资源同样如此.如下图所示:

- 当 CPU 访问 IO 设备的虚拟地址时, 首先通过 MMU 将虚拟地址 C 转换为物理地址 B, 这段物理地址应当位于该 IO 设备的 MMIO 区域. 物理地址B会通过总线(host brigde, 或者 PCIe 的 root complex)转化为总线地址A, 即可访问到设备.
- 一般device是没有发起访问系统memory的能力, device想要访问内存的意图是cpu发起的, 例如 device 通过中断告知 CPU 收到了数据, CPU 通过 DMA 的方式从将设备内存拷贝到主机内存. 总线地址 Z 会通过 IOMMU 转为为物理地址 Y, 内核只需要分配一块区域建立从虚拟地址到物理地址的映射(kmap)即可访问到这部分的数据.

![20240722015006](https://raw.githubusercontent.com/learner-lu/picbed/master/20240722015006.png)

```c
__iomem *ioremap(resource_size_t phys_addr, unsigned long size);
```

第一个参数是被映射的 physical address(就是 `/proc/iomem` 看到的那个),第二个参数给出了映射的范围,函数的返回值则是一个 virtual address

## BAR空间

> [【精讲】PCIe基础篇_BAR(Base Address Register)详解](https://blog.csdn.net/u013253075/article/details/119361574)
>
> [what is the base address register bar in pcie](https://stackoverflow.com/questions/30190050/what-is-the-base-address-register-bar-in-pcie)

系统中的每个设备中,对地址空间的大小和访问方式可能有不同的需求,例如,一个设备可能有256字节的内部寄存器/存储,应该可以通过IO地址空间访问,而另一个设备可能有16KB的内部寄存器/存储,应该可以通过基于MMIO的设备访问.

哪些地址应该使用哪种方式(IO或Memory)来访问它们的内部位置,这是系统软件(即BIOS和OS内核)的工作.因此设备必须为系统软件提供一种方法来确定设备的地址空间需求.这种需求就是是通过配置空间头部中的Base Address register (BAR)实现的.一旦系统软件知道设备在地址空间方面的需求,系统软件将分配一个适当类型(IO, NP-MMIO或P-MMIO)的可用地址范围给该设备.

![20240722011239](https://raw.githubusercontent.com/learner-lu/picbed/master/20240722011239.png)

## MMIO

PCIe总线中有两种MMIO:P-MMIO和NP-MMIO; P-MMIO,即可预取的MMIO(Prefetchable MMIO);NP-MMIO,即不可预取的MMIO(Non-Prefetchable MMIO).其中P-MMIO读取数据并不会改变数据的值.


在内核中如果要对某些端口进行操作,就要首先获取到访问该IO权限,以防止其他程序同时操作该端口.要获取端口端口权限可以使用`request_region` 函数,该函数定义 `include/linux/ioport.h` 文件中:

从CPU角度看到的地址是虚拟地址,要访问对应的物理地址需要页表将虚拟地址与物理地址映射起来.

```c
struct resource *request_region(unsigned long first, unsigned long n, const char *name);
// first:要获取的起始端口.如果要同时获取多个连续端口,则该参数为起始端口
// n: 要获取端口数量
// name:设备名字
```

当获取到端口之后,可以在 `/proc/ioports` 文件中查看当前系统所有已经被分配的端口.

```bash
(base) kamilu:~/klinux$ sudo cat /proc/ioports
0000-001f : dma1
0020-0021 : pic1
0040-0043 : timer0
0050-0053 : timer1
0060-0060 : keyboard
0064-0064 : keyboard
0070-0071 : rtc0
0080-008f : dma page reg
00a0-00a1 : pic2
00c0-00df : dma2
00f0-00ff : fpu
0400-0403 : ACPI PM1a_EVT_BLK
0404-0405 : ACPI PM1a_CNT_BLK
0408-040b : ACPI PM_TMR
040c-040f : ACPI GPE0_BLK
```

当端口使用完毕或者驱动模块卸载时,需要将占用的端口给释放掉,以供其他程序使用,释放端口函数为 `release_region()` 函数:

```c
void release_region(unsigned long start, unsigned long n);
```

从DMA角度看到的地址是总线地址,DMA的主要工作是负责设备与物理内存数据的搬运.当数据需要从物理内存搬运到设备时,物理地址B会通过host brigde转化为总线地址A,即可访问到设备.当数据从设备到物理内存搬运,总线地址Z会通过IOMMU转为为物理地址Y,即可访问物理内存.当CPU要访问设备时,虚拟地址与物理地址B通过ioremap进行映射,再通过host bridge访问到总线地址A,这样就建立起C->B->A的访问

`ioremap` 是一种内核函数,用于将**设备的物理地址空间映射到内核的虚拟地址空间**,使CPU可以通过访问内核虚拟地址来访问设备寄存器或内存. 当调用 `ioremap` 时,内核会分配一段虚拟地址空间,并将其映射到设备的物理地址. 该映射通常是非缓存的,以确保对设备寄存器的访问是直接的

## 参考

- [Linux - MMIO 的映射和访问](https://zhuanlan.zhihu.com/p/609594794)
- [stackexchange unix](https://unix.stackexchange.com/questions/741710/how-does-mmio-get-routed-to-io-devices)
- [what is the base address register bar in pcie](https://stackoverflow.com/questions/30190050/what-is-the-base-address-register-bar-in-pcie)
- [【精讲】PCIe基础篇_BAR(Base Address Register)详解](https://blog.csdn.net/u013253075/article/details/119361574)