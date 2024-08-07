
# pcie

PCI总线无疑是总线中的王者.在它之前,各种平台都拥有自己特定的总线,例如x86的ISA总线、Power PC的VME 总线.PCI出现后,由于速度快、具有动态配置功能和独立于CPU架构等特点,迅速被各种平台接受,成为一种通用的总线架构

在 [总线](../arch/bus.md) 中我们简要介绍了从 ISA 到 PCI 的发展历史, 本文我们来进一步介绍一下 PCI 总线的相关知识, 这里会跳过硬件细节的部分专注于软件管理

## PCI总线架构

PCI总线是一种典型的树结构.把北桥中HOST-PCI桥看作根,总线中其他PCI-PCI桥、PCI-ISA桥(ISA总线转PCI总线桥)等桥设备和直接接PCI总线的设备看作节点,整个PCI架构如下所示

![20240722163540](https://raw.githubusercontent.com/learner-lu/picbed/master/20240722163540.png)

通过桥,PCI总线可以很容易地被扩展,并且与其他总线相互挂接,构成整个系统的总线网络.与HOST-PCI相连的总线被称为总线 0,其他层次总线的编号,是在BIOS(或操作系统)枚举设备时确定的,这会在下文介绍

### 设备标识符

「设备标识符」 可以看作是设备在PCI总线上的地址, 它的一般表示格式为

```txt
<bus>:<device>.<function>
```

- Bus字段代表设备所在的总线号, 该字段 8 位, 故系统中最多有256条总线.
- Device字段表示设备号, 该字段 5 位, 代表在Bus所表示总线上的某个设备.
- Function字段表示功能号,标识具体设备上的某个功能单元. 该字段 3 位. "功能单元"这样的说法很口,实际上可以称为**逻辑设备**.

> 举一个简单的例子,一块PCI卡,它上面有两个独立的设备,这两个设备共享了一些电子线路,那么这两个设备就是这块PCI卡的两个功能单元.但从软件的角度来看,它们和两个独立接入PCI总线的设备无异.如同Function字段长度(3)所暗示的,**一个独立的PCI设备上最多有8个功能单元**.Device和Function两个字段一般合起来使用,表示一条总线上最多有 256个设备.

通常,用设备标识符三个字段的缩写**「BDF」(bus/device/function)**来代表它.当程序通过BDF访问某个设备时,先通过Bus字段选定特定的总线,再根据Device字段选定特定的设备,最后通过Function字段就可以选定特定的功能单元(逻辑设备)了.

例如可以使用 `lspci -vv` 查看系统中 PCI 设备的详细信息, 其中 `Control` 字段的 `I/O-` 说明这不是一个启用 IO port 方式的 PCI 设备, 即采用的是 MMIO 的映射方式.

![20240722180732](https://raw.githubusercontent.com/learner-lu/picbed/master/20240722180732.png)

上图的系统是笔者的 WSL2, 采用的是虚拟化方案因此其 BDF 字段比较特殊. 对于普通的主机来说可以看到标准的 BDF

![20240722180932](https://raw.githubusercontent.com/learner-lu/picbed/master/20240722180932.png)

### 配置空间

对于程序员来说,不需要了解PCI设备电路上实现的细节,只需要了解操作它的接口即可.PCI配置空间正是这么一个接口, 其结构如下图所示.

PCI设备规范规定,设备的配置空间最多为256个字节,其中前64个字节的格式和用途是统一的. 我们这里不去详细展开, 各个字段的具体含义请参见"PCILocalBus SpecificationRevision 3.0"的第6章. 

![20240722171827](https://raw.githubusercontent.com/learner-lu/picbed/master/20240722171827.png)

这里只关心对于程序员最重要的三个字段

- 「Base Address Registers」: 基地址寄存器,也就是常说的PCI Bar.它报告设备寄存器或设备 RAM 在I/O端口地址空间(或物理地址空间中)的地址.**地址是由软件(BIOS或操作系统)动态配置的**,这就一改ISA设备通过跳线进行配置的不灵活的特点.
  
  通常枚举PCI设备的软件(BIOS或操作系统)会在获得平台所有PCI设备后,根据设备数量,依照固定的算法为每个设备的PCIBar分配I/O端口(或物理地址).设备的电子线路负责把这些端口(或地址)映射到自身的寄存器(设备RAM)上,这样,CPU就可以通过端口号(PortI/O方式)、物理地址(MMIO方式)访问到设备了.
  
  使用哪种方式访问,由PCI Bar的最后一个位表示.当该位为1时,表示是I/O端口;该位为0时,表示是MMIO端口.某些架构根本就没有PortI/O方式,全部采用MMIO(例如 ARM RISCV MIPS)

  PCI Bar又可以划分为如下两种类型.

  - 可预取(Prefetchable)类型:这主要是**设备RAM**.由于RAM具有在每次读操作后内容不自动改变的性质,所以可以使用预读机制.例如,程序在读第N个字节的内容时,总线可能已经读出了第N十1个字节的内容.当预读出的内容不需要时,只要简单地抛弃就行,不会有什么影响
  - 不可预取类型(Un-Prefetchable):这主要指**设备寄存器**.寄存器和RAM有着不同的性质,有些寄存器本身就是设备的FIFO队列的接口.很可能当一次读操作完成后,寄存器的值就改变了.如果使用预读机制,例如程序本身只读了寄存器的第一个字节,而总线却连续读人了4个字节,那么后面3个字节的内容可能就会改变,下次程序真正访问它们时得到的就是错误的值.对于PCI Bar是否为可预取类型,可以根据该PCIBar的第3个位判断,1为可预取,否则为不可预取

  ![20240722174127](https://raw.githubusercontent.com/learner-lu/picbed/master/20240722174127.png)

  > 上图1是Memory BAR寄存器的结构(可预取),图2是IO类型的BAR寄存器(不可预取)

- Interrupt Pin: 中断针脚.PCI中断线的标准设计是4条:INTA、INTB、INTC和INTD,分别对应值0～3.该寄存器的值表示设备连接的是哪个中断针脚
- Interrupt Line: 设备的中断线.该寄存器只起一个保存作用,BIOS 和操作系统可以自由使用它.BIOS通常用它来保存设备所连的PIC/IOAPIC的管脚号

## PCI设备枚举

PCI设备的枚举和资源分配(即配置PCI配置空间)通常是由 BIOS完成的,并提供特殊的PCI设备枚举接口供保护模式下的操作系统使用,这些接口称为PCIBIOS.由于某些平台,例如嵌人式,是没有BIOS的,并且操作系统厂商对BIOS的可靠性也不信任,故某些操作系统也实现了自己的PCI设备枚举接口.无论是BIOS,还是操作系统,其枚举设备的过程都遵循着一般规律

从前面的PCI总线概要图知道,PCI设备和总线一起构成了树结构,其中PCI-PCI桥(或PCI-ISA等其他桥,这里只关心PCI-PCI桥)北桥是子树的根节点,设备枚举的过程就是要在内存中建立一棵和实际总线情况相符合的设备树.枚举过程中最关键的步骤是发现PCI-PCI桥,这个可以通过PCI配置空间的Header Type字段判断,该字段为1时表示为桥设备.PCI-PCI桥主要有三个属性.

- Primary Bus:表示该桥所属的根总线
- Secondary Bus:表示以该桥为根节点的子总线
- Subordinate Bus:表示该桥为根的子树中最大的总线号

![20240722180457](https://raw.githubusercontent.com/learner-lu/picbed/master/20240722180457.png)

如上图所示. 对于"PCI-PCI桥1",其PCI设备的枚举Primary Bus 是总线0,Secondary Bus 是总线 1,而以它为根的总线中最大的总线号为2,所以其Subordinate Bus为总线2.

设备枚举从根节点HOST-PCI桥开始,首先探测总线O上的各个设备.当探测到第一个桥设备时,为其分配PrimaryBus号和SecondaryBus号,其中SecondaryBus号为1(即当前系统中最大总线号加1),SubordinateBus号暂设为和SecondaryBus相同,当在子树中发现新总线后会动态调整该值.接着以该桥为根节点,继续探测其下属总线,其过程和前面的相同,发现第一个桥设备后则以其为根往下探测,如此反复直到所有的子树都探测完毕.

通过这种方式,BIOS或操作系统可以枚举出总线上所有设备并为之分配资源, 比如通过 MMIO 的方式为一个 PCI 设备分配一段地址空间. 一旦PCI配置空间设定好,软件就可以直接通过PCI Bar访问设备了.

> 更多内容请参考 [MMIO](../mm/mmio.md)

## PCIe

PCI Express 的设计目标是用来替换当前广泛使用的PCI、PCI-X和AGP等总线标准,成为新一代通用、高速的IO互联标准,同时保持对PCI标准的软件兼容性.PCI Express标准在PCI-SIG组织的推动下不断向前发展.

PCI Express抛弃了PCI所采用的多个设备共享的并行的总线结构,转而使用了与网络协议类似的点对点的串行通信机制.多个PCI Express设备(Endpoint)通过交换器(Switch)互相连接.与PCI总线中的桥设备类似,通过交换器,可以搭建一个树形的PCI Express 的拓扑结构.标准的PCI Express 拓扑结构如下图所示.树的根节点是`Root Complex`,用来连接处理器、内存系统和IO系统,其作用类似PCI总线树中的Host-PCI桥

## 参考

- [PCIe扫盲](http://blog.chinaaet.com/justlxy/p/5100053328)
- [【精讲】PCIe基础篇_BAR(Base Address Register)详解](https://blog.csdn.net/u013253075/article/details/119361574)
- [what is the base address register bar in pcie](https://stackoverflow.com/questions/30190050/what-is-the-base-address-register-bar-in-pcie)
- [File:Pci-config-space.svg](https://en.wikipedia.org/wiki/File:Pci-config-space.svg)