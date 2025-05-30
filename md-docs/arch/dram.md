
# dram

文章开始之前先推荐一个B站的3D科普视频 [计算机是如何工作的?探索主内存,以DDR5为例](https://www.bilibili.com/video/BV1vP411c7pt)

## 随机访问存储器

随机访问存储器(Random Access Memory, RAM)分为两类,静态和动态的.静态RAM(SRAM)比动态RAM(DRAM)更快,也贵得多.SRAM用来作为高速缓冲存储器,既可以在CPU芯片上,也可以在片外.DRAM用来作为贮存一级图形系统的帧缓冲区.

### 静态RAM

SRAM将每个为存储在一个双稳态的存储器单元,每一个单元使用一个六晶体管电路来实现的,这个电路有这样一个属性,它可以无限期的保持在两个不同的电压配置或者状态之一.其他任何状态都是不稳定的.

从不稳定的状态开始,电路会迅速的转移到两个稳定的状态中的一个.

![20221223205730](https://raw.githubusercontent.com/learner-lu/picbed/master/20221223205730.png)

由于SRAM存储器单元的双稳态特性,只要有电就会保持它的值,即使有干扰来扰乱电压,当干扰消除时电路就会恢复到稳定的值.

### 动态RAM

DRAM将每个为存储为一个对电容的充电,这个电容非常小,通常只有大约30x10^-15F. DRAM存储器可以制造的非常密集,每个电源有一个电容和一个访问晶体管组成,但是与SRAM不同,DRAM存储器单元对干扰非常敏感,当电容的的电压被扰乱之后,它基本就永远不会恢复了.暴露在光线下会导致电容电压的改变.

> 实际上数码照相机和摄影机中的传感器本质就是DRAM单元的阵列

很多原因会导致漏电,值得DRAM单元在10-100毫秒时间内失去电荷,幸运的是计算机运行的时钟周期都是纳秒来衡量的.所以相对而言这个保持时间是比较长的,内存系统必须周期性的通过读出,然后重写来刷新内存的每一位.有些系统也是用纠错码,其中计算机的字会被多编码几个位.这样电路可以发现并且纠正一个字中任何单个的错误位.

下图总结了SRAM和DRAM存储器的特性,只要有点,SRAM就会保持不变,与DRAM不同SRAM不需要刷新.SRAM的存取更快,对光电噪声干扰不敏感.带解释SRAM单元比DRAM单元使用更多的晶体管,因为密集度低,更贵,功耗更大

![20221223210953](https://raw.githubusercontent.com/learner-lu/picbed/master/20221223210953.png)

## 传统DRAM

DRAM芯片中的单元被分为d个超单元(supercell),每一个超单元都有w个DRAM单元组成,一个d x w 的DRAM总共存储了dw位信息,超单元被组织成一个r行c列的长方形阵列,之类的rc=d

![20221223212747](https://raw.githubusercontent.com/learner-lu/picbed/master/20221223212747.png)

上图是一个16x8的DRAM芯片的组织,一共有d=16个超单元,每一个超单元有w=8位

被组织成r=4,c=4的格式. 图中给出了两组引脚,8个data引脚,可以传送一个字节到芯片或者从芯片传出一个字节. 以及两个addr引脚,他们携带两位的行列超单元地址,可以找到相应的超单元

{% note info %}
存储领域从来没有为DRAM的阵列元素确定一个标准的名字,计算机架构师倾向于称之为单元.使这个术语具有DRAM存储单元之意.单路设计师倾向于称之为字,使之具有主存一个字之意.这里使用 **超单元** 为了避免歧义
{% endnote %}

每个DRAM芯片被连接到一个称为内存控制器的电路,这个电路可以依次传送w位到每个DRAM芯片,或者依次从每个DRAM芯片传出w位.为了读出超单元(i,j)的内容,内存控制器将行地址i发送到DRAM,然后是列地址j. DRAM把超单元(i,j)的内容发挥控制器作为响应. 

> 行地址i称为RAS(Row Access Strobe,行访问选通脉冲)请求.
>
> 列地址j称为CAS(Column Access Strobe,列访问选通脉冲)请求.
> 
> 注意RAS和CAS请求共享相同的DRAM地址引脚

![20221223214442](https://raw.githubusercontent.com/learner-lu/picbed/master/20221223214442.png)

例如,要从上图中的16x8的DRAM中读出超单元(2,1),内存控制器发送行地址2,DRAM的响应是将行2的 **整个内容** 都复制到一个内部行缓冲区,接下来内存控制器发送列地址1,DRAM的响应是从行缓冲区复制出超单元(2,1)中的8位,并把它们发送到内存控制器中

{% note info %}
电路设计设将DRAM组织成二维阵列而不是线性数组的一个原因是降低芯片上的地址引脚数量

例如128位DRAM被组织成一个16个超单元的线性数组,地址为0-15,那么芯片会需要四个地址引脚而不是两个,二维阵列组织的确定是必须分成两步发送地址,这增加了访问时间
{% endnote %}

## 内存模块

DRAM芯片封装在内存模块,它插到主板的扩展槽上

![20221223215211](https://raw.githubusercontent.com/learner-lu/picbed/master/20221223215211.png)

上图展示了一个内存模块的基本思想,示例模块用了8个8MX9的DRAM芯片,总共存储64MB. 这8个芯片编号分别为0-7,每一个超单元存储主存的一个字节,而用相应的超单元地址为(i,j)的8个超单元来表示主存中的地址A处的64位字. **注意这里的64位字是通过8个DRAM芯片分别存储8个8位字来实现的**

要去除内存地址A处的一个字,内存控制器将A转换成一个超单元地址(i,j), 并将它发送到内存模块,燃火内存模块再将i,j广播到每个DRAM,每个DRAM输出他的(i,j)超单元的8位内容,模块中的电路收集这些输出并把它们合并成一个64字,再返回给内存控制器.

## 增强的DRAM

有许多种DRAM存储器,而生产厂商为了跟上迅速增长的处理器速度,市场上就会定期推出新的种类.每种都是基于传统DRAM单元进行一些优化,提高访问基本DRAM单元的速度

- 快页模式DRAM(Fast Page Mode DRAM, FPM DRAM)

  传统DRAM将超单元的一行复制到它的内部行缓冲区,使用一个然后丢弃剩余的

  FPM DRAM允许对同一行连续的访问,可以直接从行缓冲中的得到服务.例如要从一个传统DRAM的行i连续读取四个超单元,内存控制器必须发送四个RAS/CAS请求,即使行地址i在每个情况都是一样的.

  而要从一个FPM DRAM的同一行中读取超单元,内从控制器发送第一个RAS/CAS请求,后面跟三个CAS请求,出事的RAS/CAS请求将行i复制到行缓冲区,并返回CAS那个超单元,后面的三个超单元可以直接在行缓冲区中获得,因此返回的更快

- 扩展数据输出DRAM(Extended Data Out DRAM, EDO DRAM)

  FPM DRAM的一个增强的形式,它允许各个CAS信号在时间上靠的更近一些

- 同步DRAM (Synchronous DRAM, SDRAM)

  在与内存控制器通信时使用的一组显式控制信号来说,传统DRAM FPM和EDO DRAM都是异步的.

  SDRAM用与驱动内存控制器相同的外部时钟信号的上升沿来代替许多这样的控制信号

- 双倍数据速率同步DRAM(Double Data-Rate Synchronouse DRAM, DDR SDRAM)

  DDR SDRAM是对于SDRAM的一种增强,使用两个时钟沿作为控制信号使得DRAM的速度翻倍

  不同类型的DDR SDRAM是用预取缓冲区的大小来划分的, DDR2 3 4 5

- 视频RAM(Video RAM, VRAM)

  它用在图形系统的帧缓冲区中,VRAM的思想和FPM DRAM类似,两个主要区别是

  - VRAM的输出是通过依次对内部缓冲区的整个内容进行移位得到的
  - VRAM允许对内存并行的读写,因此系统可以在新值写入的同时用帧缓冲区中的相似刷新屏幕(读)

## 访问主存

数据流通过称为总线的共享电子电路在处理器和DRAM主存之间传送.每次CPU和主存之间的数据传送都是通过一系列步骤完成的,这些步骤称为总线事务.

- 读事务:从主存传送数据到CPU
- 写事务:从CPU传送数据到主存

总线是一组并行的导线,**可以携带地址,数据和控制信号**,取决于总线的设计,**数据和地址信号可以共享同一组导线,也可以使用不同的**.**同时两个以上的设备也能共享同一总线.**控制线携带的信号会同步事务,并标示出当前正在被执行的事务的类型.例如当前关注的这个事务是到主存的么?还是到诸如磁盘控制器这样的其他IO设备的.是读还是写,总线上的信息是地址还是数据项

![20221224205731](https://raw.githubusercontent.com/learner-lu/picbed/master/20221224205731.png)

上图展示了一个示例计算机系统的配置,主要部件是CPU芯片,被称为IO桥接器的芯片组(包括内存控制器),以及组成主存的DRAM内存模块.这些部件由一对总线连接起来,其中一条总线是系统总线,他连接CPU和IO桥接器. 另一条总线是内存总线,他链接IO桥接器和主存,IO桥接器将系统总线和电子信号翻译成内存总线和电子信号.

IO桥也将系统总线和内存总线连接到IO总线,像磁盘和图形卡这样的IO设备共享IO总线

总线设计是计算机系统一个复杂而且变化迅速的方面,不同的厂商提出了不同的总线体系结构作为产片差异化的一种方法.

例如Intel系统使用北桥和南桥的芯片组分别将CPU连接到内存和IO设备

> 关于主板的南桥芯片推荐一个B站科普视频: [【硬核科普】电脑主板右下角的散热片下面究竟隐藏着什么?详解主板南桥芯片组的功能和作用](https://www.bilibili.com/video/BV1cJ411K7HW)

这些不同的总线体系结构的细节超出了存储器结构层次的返回,我们将使用上图中的高级总线体系结构作为一个运行示例贯穿时钟,这是一个简单但是有用的抽象,使得我们可以很具体,并且可以掌握主要思想而不必与任何私有设计的细节绑得太紧

考虑当CPU执行如下一个加载操作时会发生什么?

movq A, %rax

这里地址A的内容被加载到寄存器%rax中

CPU芯片上称为总线接口的电路在总线发起读事务.读事务由三个步骤组成

- 首先CPU将地址A放到系统总线上,IO桥将信号传递到内存总线

  ![20221224205751](https://raw.githubusercontent.com/learner-lu/picbed/master/20221224205751.png)

- 接下来主存感知到内存总线上的地址信号,内存总线读地址,从DRAM取出数据字,并将数据写到内存总线,IO桥将内存总线信号翻译成系统总线信号,然后然后沿着系统总线传递

  ![20221224210004](https://raw.githubusercontent.com/learner-lu/picbed/master/20221224210004.png)

- 最后CPU感知到系统总线上的数据,从总线上读数据,并将数据复制到寄存器%rax

  ![20221224210045](https://raw.githubusercontent.com/learner-lu/picbed/master/20221224210045.png)

## 连接IO设备

例如图形卡,显示器,鼠标键盘,磁盘等IO设备都是通过IO总线连接到CPU和主存的.例如Intel的PCI(Peripheral Component Interconnect)总线

下图展示了一个典型的IO总线结构,虽然IO总线比系统总线和内存总线慢,但是他可以容纳种类繁多的第三方IO设备

![20221228074613](https://raw.githubusercontent.com/learner-lu/picbed/master/20221228074613.png)

- 通用串行总线USB(Universal Serial Bus),USB总线是一个广泛使用的标准,连接各种外围设备,USB3.0总线最大带宽是625MB/s
- 图形卡
- 主机总线适配器:将一个或多个磁盘连接到IO总线,使用的是一个特别的主机总线接口定义的通信协议,两个最常用的磁盘接口是SCSI和SATA

  > SCSI磁盘通常比SATA驱动器更快但是更贵,除此之外还有一种相对新的NVMe,目前市面上最常见的SSD产品,几乎都是SATA的,主流产品的读写通常都在550MB/s,而NVMe可以轻松打破这一限制,NVMe固态硬盘价格会比SATA固态贵一些

- 其他:例如网络适配器,可以通过适配器查到主板上空的扩展槽中连接到IO总线,这些插槽提供了到总线的直接电路连接

> 上图的IO总线是一个简单的抽象,在现代系统中共享的PCI总线已经被PCIe(PCI express)总线取代,PCIe是一组高速串行,通过开关连接的点到点链路.PCIe总线最大吞吐量可到16GB/s,比PCI总下快一个数量级(PCI总线的最大吞吐量533MB/s)

## 访问磁盘

CPU使用一种称为内存映射的技术向IO设备发射命令,在使用内存映射的IO系统中,地址空间中有一块地址是为IO设备通信保留的,每个这样的地址称为一个IO端口,当一个设备连接到总线之后,它与一个或者多个端口相关联(或者它被映射到一个或多个端口)

来看一个简单的例子:

假设磁盘控制器映射到端口0xa0,随后CPU可能通过执行三个对地址0Xa0的存储指令来发起磁盘读.第一条指令是发送一个命令字以及其他参数,告诉磁盘发起一个读,第二条指令指明应该读的逻辑块号,第三条指令指明应该存储磁盘扇区内容的主存地址

![20221228084946](https://raw.githubusercontent.com/learner-lu/picbed/master/20221228084946.png)

当CPU发送请求之后,在磁盘执行读的时候他通常会做一些其他的工作.因为一个1GHz的处理器时钟周期是1ns,再用来读磁盘的16ms的时间里他潜在的可能执行1600万条指令,如果在传输进行的时候只是简单的等待什么也不做则是一种极大的浪费!

磁盘控制器收到来自CPU的读命令之后将逻辑块号翻译成一个扇区地址,读该扇区的内容,然后将这些内容直接传送到主存,**不需要CPU干涉**.设备可以自己执行读或者写总线事务而不需要CPU干涉的过程称为直接内容访问DMA(Direct Memory Access),这种数据传送称为DMA传送

![20221228085127](https://raw.githubusercontent.com/learner-lu/picbed/master/20221228085127.png)

在DMA传送完成之后,磁盘扇区的内容被安全的存储在主存以后,磁盘控制器通过向CPU发送一个中断信号来通知CPU,这个中断信号会发送到CPU芯片的一个外部引脚,导致CPU暂停它当前的工作跳转到一个操作系统例程,这个程序会记录下IO已经完成,然后将控制返回到CPU被中断的地方

![20221228085508](https://raw.githubusercontent.com/learner-lu/picbed/master/20221228085508.png)