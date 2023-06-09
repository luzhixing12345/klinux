# klinux

klinux 是一个精简的现代 linux kernel, 基于 Linux0.11 修改, 并尝试逐步加入了现代 linux kernel 的优秀新特性

本项目意在

- 学习 linux kernel 的优秀设计, 记录学习过程
- 维护一个 "可运行使用的" "最小的" "现代" linux内核

## 写在前面

因为本文档是关于 Linux 的学习文档, 假定读者已经具备扎实的前置知识, 因此一些较为基础的知识概念就不再赘述了.

鉴于笔者水平有限, 文中难免出现语焉不详之处, 望读者海涵. 欢迎您在 [issue](https://github.com/luzhixing12345/klinux/issues) 中留言指出文中不当之处

笔者正式加入 Linux kernel 的学习之际是 Linux6.3 版本

## 文档内容

本文档用于记录三部分内容

- Linux源码阅读

  这部分主要是关于 Linux0.11 版本的源码阅读, 尽管 0.11 版本和当前版本(6.3)差距很大, 但直接阅读 6.3 版本的代码并不现实. 笔者选择这个版本一个原因是0.11的代码量还勉强可以令人接收, 在此基础上理解并修改以开发自己的 klinux 操作系统内核; 第二个原因是有很多关于 linux0.11 版本的书籍, 作为一个初学者这对入门来说很友善

  阅读源码的目的是解决一个问题: 我已经了解了操作系统的运行工作原理, 所以 Linux 是怎么做的?

- 前期准备

  这部分主要介绍关于 linux kernel 开发的相关信息, 编译和调试内核, 开发环境配置等

  如果读者尚且对 linux 的开发环境并不熟悉建议阅读此部分

- 新功能实现: 一些内核功能/特性的介绍及实现

  这部分主要是在 Linux0.11 的基础上笔者所做的所有修改的记录, 笔者打算尝试实现一些 Linux 中已有的功能并加入 klinux 当中. 笔记主要记录三方面的内容: 这个功能是什么? Linux 是怎么做的? 我是怎么做的?

  关注现代 Linux 操作系统内核中的特性的实现

## 资源

笔者将一些相关的软件/电子书等资源均保存在 [resources](https://github.com/luzhixing12345/klinux/releases/tag/v0.0.1) 中, 读者可根据需要自行下载, 文中对应的位置也会列出源地址, 这里仅作一个资源的备份和集中索引

- [Linux内核完全注释.pdf](https://github.com/luzhixing12345/klinux/releases/download/v0.0.1/Linux-00.pdf)
- [Linux内核源代码漫游.pdf](https://github.com/luzhixing12345/klinux/releases/download/v0.0.1/Linux-02.pdf)
- [Linux0.11源码](https://github.com/luzhixing12345/klinux/releases/download/v0.0.1/linux-0.11.tar.Z)