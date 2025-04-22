
# 给内核探险者的生存手册

学习Linux内核,像攀登一座没有终点的险峰.它不会赐予你"速成"的奖章,却会磨砺你直面本质的勇气

1. **抛弃"知识树",拥抱"混沌系统"**
   内核不是教科书里工整的知识框架,而是数千万行代码构建的有机生命体.与其试图"系统学习",不如:  
   - 以问题为锚点:从一次OOM Killer的随机杀戮,到进程调度器引发的"卡顿之谜",让真实的问题牵引你深入子系统.  
   - 与代码对话:在`fork()`的深渊里看虚拟内存如何裂变,在`ext4`的元数据中追踪文件消失的真相.代码从不说谎,但你需要用`gdb`打断点逼它开口.
   - 警惕"权威解释":当教科书说"进程是资源分配单位"时,去`task_struct`里寻找那37个嵌套结构体,真实的定义永远在代码中.  

2. **与机器共舞,而非与IDE为伴**
   内核开发是贴近硬件的探戈.你需要:

   - 在真实崩溃中成长:亲手触发一次kernel panic,在dmesg的残骸里寻找线索,比读十篇调试教程更刻骨铭心.
   - 拥抱物理机的"不完美":在虚拟化泛滥的时代,去摸摸网卡芯片的温度,听听硬盘寻道的声响,这些才是协议栈延迟的肉身真相.
   - 警惕"纸上谈兵":网络协议的理论吞吐量再惊艳,也不如亲手在网卡上抓一次奔涌的数据流.
   - 拥抱"不完美"的艺术:在`drivers/`目录下,你会看到芯片厂商提交的潦草代码, 那是商业现实与理想主义的血腥妥协.  

3. **在黑暗森林中点亮自己的火把**  
   内核的复杂性如同黑暗森林,但照亮道路的从来不是别人的教程,而是你亲手点燃的火种:  
   - 从"小破坏"开始:修改TCP的拥塞控制参数,观察ss -i的输出如何颤抖;截获一个系统调用,看进程如何在你修改的规则下挣扎.
   - 与社区共振:潜伏在LKML邮件列表,看维护者如何用一句"This doesn't scale"枪毙一个幼稚的patch,那是比算法更重要的设计哲学.  
   - 创造你的"地标":在浩瀚的文件中,留下你的调试日志(哪怕最后要#if 0掉),这些印记终将成为你穿越迷宫的绳索.

4. **保持痛苦,保持饥饿**
   如果某天你觉得自己"懂了"内核,那正是危险的开始:

   - 警惕舒适区:当你能轻松阅读TCP状态机时,去挑战RDMA的verbs抽象;当熟悉x86架构后,去RISCV的世界里重新理解原子操作的代价.
   - 与性能的幽灵博弈:用 `ebpf` 在协议栈里埋下观测点,看纳秒级的延迟如何在缓存未命中中溃散,这才是真正的性能调优启蒙.
   - 内核开发者的终极对手不是代码,而是自己对"未知"的恐惧.

三十年前,Linus在赫尔辛基的公寓里写下第一行内核代码时,他面对的不是神圣的蓝图,而是一团亟待驯服的电子混沌.今天的你也一样:  
- 忘记"征服内核"的妄念,学会在`BUG_ON()`的惊雷中舞蹈  
- 把每次`systemtap`的探测当作与内核的密语  
- 让`dmesg`里滚动的日志成为你的意识流日记  

当某天你发现:  
- 能对着`schedule()`的代码会心一笑  
- 在梦中也看到一次次 read/write 是如何穿过内核抵达磁盘  
- 开始用"我们"指代内核社区  

那一刻,你已不再是学习者, 而是成了活着的Linux编年史.  

**最后赠言**:

内核深似海,有人为优化性能,有人为探究安全,有人为突破技术边疆.找到你愿为之燃烧的领域:是实时性、网络栈、文件系统,还是虚拟化?专注一处,深耕十年,你将成为世界需要的那类极客,用代码撼动现实世界的规则.

内核探索犹如攀登冰山,90%的挑战潜藏于水面之下.若你已选择这条路,便注定与平庸背道而驰.当困惑时,不妨重读Linus的"Just for fun";当疲惫时,记得Torvalds在芬兰地下室写下第一行代码的孤勇.愿你在与内核的对话中,找到属于自己的"计算机之道".

路远且艰,与君共勉 (一名仍在路上的内核旅人)

## 关于本文

如果你刚刚接触操作系统,或者对Linux环境还很陌生,直接跳进内核的汪洋大海可能会让你迷失方向.这不是因为你不够聪明,**而是因为内核的世界需要先搭建认知的阶梯**

操作系统课程通常会从理论抽象(如进程管理、内存模型、文件系统)开始,而Linux内核则是这些理论的工程实现,充满了现实世界的复杂性和妥协. 内核代码中充满了对硬件的直接操作(内存屏障、原子指令、中断控制器),而计算机体系结构、汇编语言等知识是理解它们的先决条件. 内核开发者需要理解用户的需求,而如果你连fork()和pthread_create()的区别都不清楚,就很难体会内核API的设计哲学. 我个人建议的正确的学习路线是

1. 先成为熟练的Linux用户(文件操作、权限管理、Shell脚本).
2. 通过<操作系统>课程理解理论(进程、内存、文件系统、设备驱动).
3. 用C语言开发系统软件(比如使用linux提供的api写一个coreutil中的小工具).
4. 学习计算机体系结构(CPU、内存、I/O设备如何与OS交互).
5. 最后,再进入内核源码(从某个具体子系统开始,如进程调度或文件系统)

## 关于代码

TODO: 这部分尚未完工...

本项目的代码基于 linux v6.6, 并没有大刀阔斧的改动原本的内核源码树, 只是在一些地方加入了注释便于笔者自己阅读理解

同时本项目也包含对于内核代码大刀阔斧的修改, 以如下两种方式

- 内核模块: 内核模块可以在内核外编写, 并动态装载到内核中, 非侵入式的方式不需要修改内核源码树, 本文的大部分实验性质的代码片段都保存在 modules/ 下, 在对应的章节介绍
- Patch: 有的部分需要对内核做比较底层的修改, 没有办法或者很难通过编写内核模块完成, 因此需要直接修改内核源码树. 很多初学者在一开始对 Linux 源码带有敬畏的感觉, 不敢改动代码, 相信经过一段时间的学习读者可以逐渐理解, 熟悉, 并主动尝试修改内核代码完成自己想要的功能. 本文所有的 patch 均保存在 patches/ 下, 可以直接应用到本项目或 v6.6 源码中

  ```bash
  patch -p1 < patches/xxx.patch
  ```

关于每个模块代码和patch代码的作用详见 modules/README.md 和 patches/README.md, 所有代码修改都会对应一个实验文档

## 关于笔记

本文档主要是笔者自己学习内核过程中记录的内容, 以供日后的查阅和理解. 文中表述会尽量正式, 避免口语化. 如果读者发现有不妥之处, 欢迎在评论中反馈或者提交 issue.

文中会出现几种特殊的注释风格

> [!NOTE]
> 为笔者标注的笔记内容, 用于阅读时的参考补充

> [!TIP]
> 为笔者遇到并成功解决的一些小坑, 记录在案以备后续查阅

> 颜色鲜艳的注释很亮眼, 但是我不并喜欢频繁的使用这种夸张的注释, 显得文档一片红一片蓝反倒没有重点. 文中内容仍然是以文字辅助图片说明为主, 只有在一些重点的地方才会以这种彩色注释标注

## 参考

所谓站在巨人的肩膀上可以看得更远, 本系列当然不是笔者顿悟而出, 事实上很多文字都是整合诸多前辈文章中内容并重新组织语言凝炼而成, 参考了很多大佬的文章/博客/代码, 本系列文章参考众多, 下面只列出一些系列的参考资料, 每一篇文章的相关的参考都会标注在对应文末, 感兴趣的读者可以自行阅读

- code
  - [linux v6.6](https://github.com/torvalds/linux/tree/v6.6)
  - [linux v4.4](https://github.com/torvalds/linux/tree/v4.4)
- 书籍
  - [Linux内核完全注释](http://oldlinux.org/download/CLK-5.0-WithCover.pdf)
  - 深入linux内核架构
- 技术博客
  - [Linux技术博客文档](https://www.cnblogs.com/pengdonglin137/p/15173512.html) 很全
  - [linux-insides](https://github.com/0xAX/linux-insides)
  - [linux-insides-zh](https://github.com/MintCN/linux-insides-zh)
  - [术道经纬 专栏](https://www.zhihu.com/column/c_1108400140804726784) 绝佳
  - [Rust OS](https://os.phil-opp.com/zh-CN/)
  - [wowotech](http://www.wowotech.net/) 很好
  - [banshanjushi](https://www.cnblogs.com/banshanjushi) 很好
  - [osdev wiki](https://wiki.osdev.org/)
  - [Linux技术 专栏](https://www.zhihu.com/column/c_1445694677312245760)
  - [BSP-路人甲](https://www.cnblogs.com/jianhua1992)
  - [DF11G](https://www.cnblogs.com/DF11G)
  - [archbase document](https://foxsen.github.io/archbase/)
  - [<虚拟内存的架构和操作系统支持>笔记(一):基础](https://zhuanlan.zhihu.com/p/587353806)
  - [linuxStack](https://github.com/g0dA/linuxStack)
  - [Microarchiture](https://blog.csdn.net/hit_shaoqi/category_9791833.html)
  - [【程序人生】HelloWorld_从程序到进程](https://blog.csdn.net/huiyeruzhou/article/details/130818548)
  - [linux 技术博客](https://www.junmajinlong.com/tags/Linux/)
  - [操作系统 技术博客](https://www.junmajinlong.com/tags/OS/)
  - [linuxcatalog](https://github.com/zhangjaycee/real_tech/wiki/linuxcatalog)
  - [PCIe 扫盲](http://blog.chinaaet.com/justlxy/p/5100053251)
  - [宋宝华](https://blog.csdn.net/21cnbao?type=blog)
  - [blog kernel](https://kernel.blog.csdn.net/?type=blog)
  - [gatieme的文章](https://www.zhihu.com/people/gatieme/posts)
    - [内存管理](https://kernel.blog.csdn.net/article/details/52384965)
    - [进程管理](https://kernel.blog.csdn.net/article/details/51456569)
  - [linux 技术文档](https://arthurchiao.art/categories/)
  - [chinaunix blog](http://blog.chinaunix.net/uid/23769728.html)
  - [Linux内核学习与研究 专栏](https://www.zhihu.com/column/fishland)
  - [kernel_awsome_feature](https://github.com/0voice/kernel_awsome_feature)
  - [realwujing 技术博客](https://realwujing.github.io/tags/)
  - [小坚学Linux](https://blog.csdn.net/sinat_22338935?type=blog)
  - [archives](https://abcdxyzk.github.io/blog/archives/)
  - [RTFSC 专栏](https://www.zhihu.com/column/c_1470701277923860480)
  - [郭佳明的博客](https://gls.show/categories/)
  - [大隐隐于野](https://blog.csdn.net/weixin_43778179?type=blog)
  - [vmalloc](https://lzz5235.github.io/2015/05/26/vmalloc.html)
  - [kernel_memory_management](https://github.com/luckyq/kernel_memory_management)
  - [laumy linux](https://www.laumy.tech/category/linux)
  - [LoyenWang](https://www.cnblogs.com/LoyenWang/tag/linux/)
  - [terenceli's blog](https://terenceli.github.io/)
  - [聊聊linux内核](https://mp.weixin.qq.com/mp/appmsgalbum?__biz=Mzg2MzU3Mjc3Ng==&action=getalbum&album_id=2559805446807928833&scene=173&from_msgid=2247486879&from_itemidx=1&count=3&nolastread=1#wechat_redirect)
  - [liujunming Kernel](https://liujunming.top/categories/Kernel/)
- talk
  - [The Linux Storage, Filesystem, Memory Management & BPF Summit @ OSS NA 2023](https://www.youtube.com/playlist?list=PLbzoR-pLrL6rlmdpJ3-oMgU_zxc1wAhjS)