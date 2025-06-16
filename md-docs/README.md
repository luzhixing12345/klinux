
# klinux

学习 Linux 内核，是一场没有终点的旅程。它不会带来“速成”的喜悦，但会让你学会直面复杂，沉下心来，用代码去理解这个世界的底层逻辑。

很多人一开始都想“系统性地学内核”，但内核不像一本教科书，它是一棵枝叶繁茂、错综复杂的大树。与其试图把整棵树画在纸上，不如从一个个你真实遇到的问题出发：

* 比如某次系统卡顿、某个程序被 OOM 杀死、不明原因的内核崩溃……让这些问题带你走进调度器、内存管理、文件系统等子系统。
* 学会读代码，比如看看 `fork()` 究竟是怎么复制进程的，`ext4` 是怎么管理文件的。
* 书本上的定义不一定准确，比如“进程是资源分配单位”——想知道它到底分配了什么，还是得去翻一翻 `task_struct`。

**内核的真相永远藏在代码和运行时的行为里，而不是一张张 PPT 上。**

亲自动手，比盯着 IDE 更重要。内核学习不是看教程、敲 demo，而是跟真实机器打交道：

* 别怕 kernel panic，哪怕是你自己写代码把系统搞崩了，也能从 dmesg 里学到很多。
* 如果可以，尽量在物理机上测试。听听硬盘的声音，看看网卡怎么掉包，这些细节能让你对内核有更真实的感受。
* 不要光看理论，比如“这个协议能跑 10Gbps”，不如自己用抓包工具看它实际跑了多少。
* 内核代码也不完美，`drivers/` 目录里到处是“现实妥协”的产物，理解这些代码的“为什么”，比评价它的“好坏”更有价值。
* 从小实验开始：比如修改 TCP 拥塞算法的参数，看看对网络有什么影响；或者写个内核模块，拦截一个系统调用。
* 去看看别人怎么写代码：LKML 邮件列表是个好地方，里面有一线开发者的真实讨论，比很多教程更实战。
* 留下自己的足迹：比如在某个复杂函数里加几句打印，做几次调试注释——哪怕之后删掉，它们也帮你标记了思路。

当你开始觉得“内核好像也没那么难”，其实是到了一个该突破的阶段：

* 熟了 TCP 协议？那去研究 RDMA，看看零拷贝是怎么做的。
* 熟了 x86 架构？那就去试试 RISC-V，重新理解一次原子操作。
* 试试用 eBPF 去追踪网络延迟、缓存命中率……你会看到另一个层面的性能世界。

**你真正要战胜的，并不是代码，而是对未知的畏惧。**

三十多年前，Linus Torvalds 在赫尔辛基的一个房间里写下了 Linux 的第一行代码，他并没有一个伟大的计划，只是出于兴趣，想造点什么。当你能看着 `schedule()` 会心一笑，当你能大致说出 read/write 究竟做了什么，当你开始关注内核邮件列表里的 patch 而不是教程——你已经在路上了。

**一句赠言：**

内核这条路确实难走，但也正因为它难，才值得你去走。找到你真正关心的那个方向——无论是网络、文件系统、安全、虚拟化，还是嵌入式、调度器……只要坚持深耕，总有一天，你会成为那个解决别人解决不了问题的人。愿你保持热情、保持探索、保持自信。

**—— 来自一位仍在探索中的内核学习者**

## 关于本文

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

本文档主要是笔者自己学习内核过程中记录的内容, 以供日后的查阅和理解. 文中表述会尽量正式, 避免口语化. 如果读者发现有不妥之处, 欢迎反馈

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