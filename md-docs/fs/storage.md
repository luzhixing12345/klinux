
# storage

磁盘NCQ

当我们谈持久的时候指的是可以长时间存储数据的介质, 这也是构成一切文件的基础

> TODO 一个 coredump 和调试的示例

大约在 1955 年至 1975 年间的 20 年中, 磁芯存储器(Magnetic-core memory)是随机存取计算机存储器的主要形式

![20240225151912](https://raw.githubusercontent.com/learner-lu/picbed/master/20240225151912.png)

这种存储器采用两根或多根导线穿过每个磁芯,形成一个 `X-Y` 磁芯阵列.当超过某个阈值的电流施加到电线上时,磁芯将被磁化.通过将一根 X 线和一根 Y 线供电至所需功率的一半来选择要写入的芯线,这样只写入交叉点处的单个芯线. 每一个存储单元称为一个核心记忆(core)

当不被读取或写入时,**内核会保持它们拥有的最后一个值**,即使电源关闭也是如此.因此,它们是一种非易失性存储器.根据它的接线方式,核心内存可能非常可靠. 现在通常在计算机程序中发生重大错误时, **操作系统会将内存的全部内容保存到磁盘以供检查而产生的文件,被称为"核心转储"(core dump)**

> 通常是当程序崩溃或以其他方式异常终止时, 程序状态的其他关键部分被转储,包括处理器寄存器,其中可能包括程序计数器和堆栈指针、内存管理信息以及其他处理器和操作系统的标志和信息.快照转储(或快照转储)是计算机操作员或正在运行的程序请求的内存转储,之后程序可以继续.核心转储通常用于帮助诊断和调试计算机程序中的错误.

## 参考

- [存储设备原理:1-Bit 信息的存储 (磁盘、光盘;闪存和 SSD) [南京大学2023操作系统-P25] (蒋炎岩)](https://www.bilibili.com/video/BV1Bh4y1x7tv/)
- [How do Hard Disk Drives Work?  💻💿🛠](https://www.youtube.com/watch?v=wtdnatmVdIg)
- [你以为被时代淘汰的磁带正在卷土重来【差评君】](https://www.bilibili.com/video/BV15X4y1u7XH/)
- [scientificamerican how-do-rewriteable-cds-wo](https://www.scientificamerican.com/article/how-do-rewriteable-cds-wo/)
- [Operating system implications of fast, cheap, non-volatile memory](https://dl.acm.org/doi/10.5555/1991596.1991599)
- [Coding for SSDs – Part 1: Introduction and Table of Contents](https://codecapsule.com/2014/02/12/coding-for-ssds-part-1-introduction-and-table-of-contents/)
- [持久化内存调研](https://zhuanlan.zhihu.com/p/229211653)