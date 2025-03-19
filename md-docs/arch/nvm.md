
# nvm

## 非易失性存储器(nonvolatile memory, NVM)

非易失性存储器(NVMe)是一种半导体技术,不需要持续供电来保留存储在计算设备中的数据或程序代码

系统制造商出于各种目的使用不同类型的非易失性存储芯片.例如,一种类型的NVM可能存储诸如硬盘驱动器(HDD)和磁带驱动器等设备的控制器程序代码.另一种类型的NVM通常用于固态驱动器(SSD)、USB驱动器和数码相机、手机和其他设备中的存储卡中的数据存储

如果断电,DRAM SRAM都会丢失他们的数据,在这个意义上它们是易失的.另一方面,非易失性存储器即使是在断电之后仍然保存着他们的信息.

虽然ROM中有些类型既可以读也可以写,但是他们整体上都被称为只读存储器(Read Only Memory,ROM). ROM 是以他们能够被重编程的次数和对他们进行重编程所用的机制来进行区分的

- PROM(Programmable ROM, 可编程ROM)只能被编程一次.
- 可擦写可编程ROM(Erassable Programmable ROM,EPROM)有一个透明的石英窗口,允许光到达存储单元.紫外线光照射过窗口,EPROM单元就被清除为0,对EPROM变成是通过使用一种把1写入EPROM的特殊设备来完成的.RPROM能够被擦除和重编程的次数和数量级可以达到1000次.

  ![20221224201328](https://raw.githubusercontent.com/learner-lu/picbed/master/20221224201328.png)

- 电子可擦除PROM(Electrically Erassable PROM,EEPROM)类似EPROM,但是它不需要一个物理上独立的编程设备,因此可以直接印制电路卡上编程,可编程次数道道10^5次
- 闪存(Flash memory)是一类非易失性存储器,基于EEPROM,它已经成为了一种重要的存储技术

  闪存无处不在,为大量的电子设备提供快速而持久的非易失性存储,包括数码相机,手机,音乐播放器,笔记本和台式机和服务器等计算机系统

  其中一种新兴的基于闪存的磁盘驱动器,成为**固态硬盘**(Solid State Disk,SSD)能提供相对于传统旋转磁盘的一种更快速更强健更低能耗的选择

存储在ROM设备中的程序通常被称为 **固件(firmware)**. 当一个计算机系统通电以后,他会运行存储在ROM中的固件,一些系统在固件中提供了少量的基本输入输出函数.例如PC的BIOS.复杂的设备,像图形卡和磁盘驱动控制器,也以来固件来翻译来自CPU的IO请求

### 非易失性存储器和易失性存储器的区别

企业和客户端计算机系统通常结合使用易失性和非易失性内存技术,每种内存类型都有其优点和缺点

例如,SRAM比DRAM快,非常适合高速缓存.DRAM是SRAM的后继者,与主动模式下的SRAM相比,其生产成本更低且所需的功率更低.DRAM的一个常见用例是存储计算机处理器运行所需的主要程序代码(内存)

非易失性NAND闪存在写入和读取数据方面比DRAM和SRAM慢.然而,NAND闪存的**生产成本远低于**DRAM和SRAM,这使得该技术更适合企业系统和消费设备中的**持久数据存储**.

非易失性存储器和非易失性存储器表达(NVMe)听起来相似,但它们的含义不同且不同.

- NVM是一种出现于1940年代后期的半导体技术
- NVMe是一种主机控制器接口和存储协议,由技术供应商联盟于2009年开始开发

NVM主机控制器接口工作组于2011年3月1日发布了1.0NVMe规范.NVMe旨在通过计算机的PCIe总线加速主机系统和SSD之间的数据传输.NVMe支持使用不同类型的非易失性存储器,例如NAND闪存和英特尔和美光开发的3DXPoint技术.NVMe是分别用于SAS和SATA驱动器的小型计算机系统接口(SCSI)标准和高级技术附件(ATA)标准的替代方案.

- NVMe使用的CPU指令数量少于SCSI和ATA命令集的一半
- 与基于SAS和SATA的SSD相比,基于NVMe的PCIeSSD具有更低的延迟、更高的IOPS和更低的功耗

### DIMM

首先讲到内存,许多用户都知道DDR3,DDR4这些但是对于DIMM和SDRAM却不太了解

DIMM(Dual Inline Memory Module,双列直插内存模块),是在单列直插存储器模块(single inline memory module,SIMM)的基础上发展起来的,SIMM提供32位数据通道,而DIMM则提供了64位的数据通道

- SDRAM:Synchronous Dynamic Random Access Memory,同步动态随机存储器.
- DDR(Double Data Rate)系列,严格意义上讲,应该是Double Data Rate SDRAM内存,也就是和说在SDRAM基础上改进的版本,只不过日常生活中,通常被简略的直接称作DDR内存

而无论是SDRAM还是DDR,实际上都是DIMM内存. **DIMM是指针脚插槽,也就是物理结构方面的分类**;而SDRAM和DDR都是**内部技术方面的分类**

## 新型非易失性存储器

非易失性存储器的例子包括只读存储器(ROM)、闪存、大多数类型的磁性计算机存储设备(例如硬盘、软盘和磁带)、光盘和早期的计算机存储方法,如纸带和打孔卡

近些年来,不论在学术界还是工业界,新型非易失存储技术都是关注的重点且取得了一定的突破.新型非易失存储由于性能相比闪存提升巨大,达到了接近DRAM的水平,一般在学术界被称为Non Volatile Main Memory (NVMM)

NVMM是对于多种新型非易失存储介质的统称,目前NVMM包括

- 相变存储器(Phase-Change Memory, PCM)

  通过硫系化合物非晶和多晶之间的快速相变来实现信息存储

- 忆阻器 Memristor(ReRAM)

  基于一种特殊的二氧化钛材料实现纳米级忆阻器实现数据存储

- 自旋扭矩转换随机存储器 (Spin Transfer Torque - Magnetic Random Access Memory,STT-MRAM)

  物理机制是磁致阻变效应

以上新型非易失存储介质所用材料虽然各有不同,但都有着低延迟、高密度、可字节寻址等优异特性.相比基于浮栅晶体管的NAND Flash确实可称得上是革命性创新

NAND Flash可以做成不同接口和不同形式的闪存设备:SATA接口一般用于普通固态硬盘(SSD)产品、PCIe接口则常用于高端服务器闪存产品.NVMM由于可以媲美DRAM的性能,可以做成类似内存条形式直接插在主板的DIMM插槽上.这类产品可以称为持久化内存,Persistent Memory (PM),或存储级内存,Storage Class Memory (SCM).PM和SCM基本是相同的含义,个人理解的细微差别是应用场景的区别_PM没有明确指向性,SCM则是倾向于处在存储层次中DRAM和闪存之间.

### NVDIMM

与DIMM类似,是基于NAND Flash的非易失型内存条. 通常被做成"电池+NAND Flash+DRAM"的形式:在掉电时用电池电量将DRAM数据刷回NAND Flash实现持久化

## 新型非易失存储的定位

近两年,英特尔推出了目前唯一的NVMM商用产品_基于3D Xpoint (Cross Point)技术的Optane Memory设备,又称Apache Pass,也可简称为AEP

业界在新型非易失存储上搞的热火朝天,性能到底如何呢?目前Optane Memory的读延迟大约是350 ns,这是一个比较让人"亦可赛艇"的数值了.要知道DRAM的读延迟大约是100 ns,至少已经看到了DRAM的尾灯(在一个数量级上).参考下图中的存储层次体系,3D Xpoint确实已经一只脚迈进了主存的区间,甩了闪存一大截

![20221225162409](https://raw.githubusercontent.com/learner-lu/picbed/master/20221225162409.png)

存储器件的定位和用途一般要考虑延迟、寿命、成本、容量、可持久化等多方面的因素,主存(Memory)和辅存(Storage)之间有很大的特性差别:

- 主存要求极低延迟、字节寻址
- 辅存要求大容量、持久化

主存和辅存是上下层级关系,而新型非易失存储的出现让我们看到了同时满足低时延、字节寻址、持久化、大容量的理想存储形态

![20221225162545](https://raw.githubusercontent.com/learner-lu/picbed/master/20221225162545.png)