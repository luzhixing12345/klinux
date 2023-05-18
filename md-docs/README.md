# klinux

klinux 是一个精简的现代 linux kernel, 基于 Linux0.11 修改, 并尝试逐步加入了现代 linux kernel 的优秀新特性

本项目意在

- 学习 linux kernel 的优秀设计, 记录学习过程
- 维护一个 "可运行使用的" "最小的" "现代" linux内核

## 写在前面

因为本文档是关于 Linux 的学习文档, 假定读者已经具备扎实的计算机前置知识, 因此一些较为基础的知识概念就不再赘述了.

鉴于笔者水平有限, 文中难免出现语焉不详之处, 望读者海涵. 欢迎您在 [issue](https://github.com/luzhixing12345/klinux/issues) 中留言指出文中不当之处

笔者正式加入 Linux kernel 的学习之际是 Linux6.3 版本

## 文档内容

本文档用于记录三部分内容

- 前期准备: 关于 linux kernel 的一些事情, 编译和调试内核, 开发环境配置等
- Linux 源码阅读
- 新功能实现: 一些内核功能/特性的介绍及实现
