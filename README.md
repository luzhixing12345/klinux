# klinux

linux 内核技术文档: [klinux document](https://luzhixing12345.github.io/klinux/)

本文档记录有关操作系统内核的技术细节

## 写在前面

阅读内核代码的主要有三种人: reader writer hacker, 对不同的人有不同的含义,学习方法、侧重点、投入的精力也大不相同

- reader 就是了解某个功能在内核的大致实现, how does it work,一般不关心某些极端情况下(内存不足、受到攻击)的处理方法,对于看不懂的地方也可以跳过. 而且读一个函数一般只看主干(happy path),不管 security/debugging/tracing,经常忽略错误处理分支
- writer 是给内核加feature和改bug的人,需要更进一步的知识,写代码要考虑很多, 哪些地方需要加锁, 失败了如何释放资源等等
- hacker 是通过分析代码找出安全漏洞并加以利用的人,研究 how to break it,读代码恐怕更注意找出error handling分支没有覆盖的case.

内核的接口稳定,但具体实现变化快, 而且比较注重代码的通用性和复用性,要照顾那些虽然你用不到但少数人会用的需求(比如个人电脑几乎用不到但是服务器几乎必备的numa), 原本简单直接的做法上增加间接层, 反而加大了理解代码的负担, 很多时候函数指针回调函数传来传去, 没有 debugger 很难看清楚怎么调用的, 而且 kernel 加了 O2 优化 debugger 还不太好使

如果你本身就要从事内核开发,那么以上这些都不是问题.对于这用户态写server的人,学内核的目的是什么,学到的知识能不能/要不要/如何用到日常开发中,这是值得思考的.

阅读内核源码绝非彰显技术实力的勋章,而是解决特定问题的工具.如果您在用户态开发中从不需要处理千万级并发,没有遇到必须修改内核才能突破的性能瓶颈,或许把精力集中在应用层优化会是更务实的选择.技术学习最珍贵的不是知识的广度,而是对自身真实需求的清醒认知.希望每位开发者都能找到最适合自己的成长路径. 我并不赞成逢人就推荐阅读 Linux 内核源码, 希望读者在阅读本项目其他文章之前了解自己需要什么. 

> [如何阅读 linux 内核代码](https://www.zhihu.com/question/20541014/answer/93312920)

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

所谓站在巨人的肩膀上可以看得更远, 本系列当然不是笔者顿悟而出, 事实上很多文字都是整合诸多前辈文章中内容并重新组织语言凝炼而成, 参考了很多大佬的文章/博客/代码, 每一篇文章的相关的参考文章都会标注在对应文末的参考中, 感兴趣的读者可以自行阅读

## 参考

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