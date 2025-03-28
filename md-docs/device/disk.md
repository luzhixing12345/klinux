
# disk

毫不夸张的说, 硬盘堪称机械工程的一个奇迹!

![disks](https://raw.githubusercontent.com/learner-lu/picbed/master/disks.gif)

> 一个非常好的硬盘机械结构可视化的视频: [How do Hard Disk Drives Work?  💻💿🛠](https://www.youtube.com/watch?v=wtdnatmVdIg)

## 硬盘结构

硬盘由包括磁盘、主轴、头堆叠组件、滑块和读写头等很多个部分组成, 其中最重要的是两个部件是**磁盘**和**磁头**

![20240415171941](https://raw.githubusercontent.com/learner-lu/picbed/master/20240415171941.png)

磁盘固定在主轴上,由中心的无刷直流电机驱动,以7200转/分钟的速度旋转.头堆叠组件位于每个磁盘的上下两端,包含一个滑块和读写头.滑块利用磁盘旋转产生的气流,使读写头悬浮在磁盘表面仅15纳米的高度,实现精确读写.

![20240507104100](https://raw.githubusercontent.com/learner-lu/picbed/master/20240507104100.png)

磁头的音圈电机由线圈和上下两个磁铁组成,用于移动臂架组件.当电流通过线圈时与磁铁相互作用产生力,使读写头在磁盘上移动.反向电流使读写头反向移动.通过精确控制电流,读写头的位置可以在30nm范围内调整.这种强大的马达设计使轻型臂架组件和读写头能够以每秒20次的速度在盘片上快速移动,并进行微小的调整.

![20240507114439](https://raw.githubusercontent.com/learner-lu/picbed/master/20240507114439.png)

磁盘包含很多个盘片, 每一个盘片有**两面**, 在逻辑上被划分为磁道、柱面以及扇区.

- 磁盘在格式化时被划分成许多**同心圆**,这些同心圆轨迹叫做**磁道**(Track),磁道从外向内从0开始顺序编号;
- **所有盘面上的同一磁道构成一个圆柱**,通常称做**柱面**(Cylinder),每个圆柱上的磁头由上而下从"0"开始编号;
- 每个磁道会被分成许多段**圆弧**,每段圆弧叫做一个**扇区**,扇区从"1"开始编号,每个扇区中的数据作为一个单元同时读出或写入,操作系统以扇区(Sector)形式将信息存储在硬盘上,每个扇区包括**512**个字节的数据和一些其他信息

![20240507104130](https://raw.githubusercontent.com/learner-lu/picbed/master/20240507104130.png)

这里会带来一个问题, **不同磁道的扇区数是否相同?** 上图是旧的记录方式,的确每个磁道所拥有的扇区数量都是一样的. 但是我们注意到随着半径的增加, **越外侧扇区的面积越大**, 同时因为主轴旋转的角速度不变, **越外侧转动的速度越快**(线速度大). 因为每个扇区所能容纳的数据量是相同的,都是512字节,而数据量需要平均分配在扇区面积的每个角落,所以外面扇区的数据密度低,里面扇区的数据密度高

![20240507110234](https://raw.githubusercontent.com/learner-lu/picbed/master/20240507110234.png)

这样结果是浪费了外围扇区的大面积,所以有了另外一种解决方案,分区记录方式 (zone bit recording, [ZBR](https://en.wikipedia.org/wiki/Zone_bit_recording)). 新的解决方式认为,既然磁盘越往外面积越大,那就应该划分出更多的扇区,每个扇区的面积都是一样的,容纳的数据量也是一样的. 可以看到下图中外侧的扇区数可以进行倍增.

![20240507110638](https://raw.githubusercontent.com/learner-lu/picbed/master/20240507110638.png)

下图是一个磁盘的测试结果, 其中横坐标代表磁道位置, 0% 表示最外侧, 100% 表示最里侧, 红线和蓝线分别代表写速度和读速度. 可以很明显的验证我们的结论: 越靠近内侧读写速度越慢

![20240508104626](https://raw.githubusercontent.com/learner-lu/picbed/master/20240508104626.png)

为什么硬盘用久了读写速度会变慢? 因为磁盘的写入的方式是从外到内,先写满最外的磁道,接着再写里面的磁道. 前面我们提到因为主轴旋转的角速度不变, 所以**越外侧转动的速度越快**(线速度大), 因此同样的时间,**读写头在外面磁道扫过的扇区数量要更多**, 反映出来的是就是**读写速度更快**. 为了解决这个问题可以进行磁盘整理或者格式化,这会使外面的磁道得到使用

## 读写

早期的纵向磁记录技术中,磁性颗粒的易磁化方向与磁盘**平行**,颗粒沿圆周排列,可能出现SS和NN的互斥排列.随着颗粒尺寸减小和密度增加,颗粒对热运动敏感,易失去稳定性,导致数据损坏.

![20240327202619](https://raw.githubusercontent.com/learner-lu/picbed/master/20240327202619.png)

由于现今硬盘的资料记录区块大小已经发展到宽度只有30纳米左右,而磁性记录颗粒的尺寸不断下降,但硬盘容量倍数上升,导致出现电磁学上的超顺磁效应(超顺磁性),大大降低了硬盘的容量提升潜力.因此为了配合电脑技术发展,有公司开始发展运用**垂直磁记录技术**的硬盘

> 超顺磁性是指当某些具有磁性的颗粒小于某个尺寸时,外场产生的磁取向力太小而无法抵抗热扰动的干扰,而导致其磁化性质与顺磁体相似

垂直磁记录技术通过使颗粒的易磁化方向与磁盘**垂直**,使用单极磁头和软磁层产生的间隙磁场写入信息,比纵向技术产生的磁场强两倍.这允许使用更强磁各向异性的材料,提高存储密度.

通过改变电流方向可以控制磁头的磁场方向, 进而影响到下方磁畴磁性方向来实现写入数据. **虽然使用 N-S/S-N 来区分 1bit 的 0/1 比较简单, 但其实情况并非如此**. 垂直磁记录技术中,读取头被设计为**检测磁场方向的变化**.相邻的磁性区域在切换方向时产生的磁场变化远强于单一方向的磁场,因此读取头可以准确识别这些变化.每个磁场方向的变化(从指向一个方向到指向相反方向)被编码为1,而从一个磁性区域到下一个相邻区域的过渡则被编码为0,从而实现了数据的读取.

![20240507125321](https://raw.githubusercontent.com/learner-lu/picbed/master/20240507125321.png)

## 寻址

CHS(Cylinder-Head-Sector)是一种早期硬盘寻址方法,通过垂直的磁头、水平的圆柱体和角坐标的扇区构成三维坐标系来定位数据块.磁头选择盘片的一个表面,圆柱体代表盘片堆叠的圆柱形交叉点,而扇区则细分磁道为等大小的弧形部分.CHS寻址因早期硬盘缺乏嵌入式控制器而设计,要求操作系统了解硬盘的物理几何以正确寻址.

随着硬盘技术进步,CHS寻址因容量限制、效率问题、技术发展、寻址简便性、兼容性问题和物理结构变化而逐渐被LBA(Logical Block Addressing)寻址取代.LBA将三维寻址转换为一维线性寻址,提高了系统效率,适应了更大容量硬盘的需求.现代计算机和硬盘接口已不再使用CHS概念,而采用LBA寻址.

## 调度算法

> [磁盘调度算法FCFS、SSTF、SCAN、CSCAN](https://www.cnblogs.com/chen-cs/p/13180242.html)
> 
> [磁盘调度算法剖析(FIFO、SSTF、SCAN、CSCAN、FSCAN)](https://blog.csdn.net/Jaster_wisdom/article/details/52345674)

NCQ(native command queuing) 磁盘控制器重排序, **数据库交易日志**(关键数据)

> read ahead 预读取+缓存

## 性能调优

查看磁盘剩余空间, 发现 / 已满

```bash
$ df -h
Filesystem      Size  Used Avail Use% Mounted on
tmpfs            76G  3.7M   76G   1% /run
/dev/nvme1n1p2  1.5T  1.4T  2.6G 100% /
tmpfs           378G     0  378G   0% /dev/shm
tmpfs           5.0M     0  5.0M   0% /run/lock
efivarfs        512K  212K  296K  42% /sys/firmware/efi/efivars
tmpfs           378G     0  378G   0% /run/qemu
/dev/nvme1n1p1  511M  6.1M  505M   2% /boot/efi
tmpfs            76G  100K   76G   1% /run/user/100
```

查看一下 / 下最大的前 20 个文件/目录

```bash
sudo du -ah / | sort -rh | head -n 20
```

> [!TIP]
> 如果执行报错 sort: write failed: /tmp/sortneOfJZ: No space left on device 说明 /tmp 目录都已经没有空间了, 执行
>
> ```bash
> sudo rm -rf /tmp/*
> ```

## 参考

- [存储器系统](https://zhuanlan.zhihu.com/p/105388861)
- [不同磁道的扇区数是否相同?的回答](https://www.zhihu.com/question/20537787/answer/77591552)
- [硬盘结构(图文展示)](https://www.cnblogs.com/cyx-b/p/14095057.html)
- [存储设备原理:1-Bit 信息的存储 (磁盘、光盘;闪存和 SSD) [南京大学2023操作系统-P25] (蒋炎岩)](https://www.bilibili.com/video/BV1Bh4y1x7tv/)
- [Cylinder-head-sector](https://en.wikipedia.org/wiki/Cylinder-head-sector)