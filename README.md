# klinux

klinux 是一个精简的现代 linux kernel, 基于 Linux0.11 修改, 并尝试逐步加入了现代 linux kernel 的优秀新特性

本项目意在

- 学习 linux kernel 的优秀设计, 记录学习过程
- 维护一个 "可运行使用的" "最小的" "现代" linux内核

本项目会持续关注 [内核架构] [内存] [进程] [网络] [驱动] [文件系统] 相关进展, 期望与最新版本功能保持同步

## 编译

```bash
sudo apt install qemu qemu-system qemu-kvm
```

https://github.com/luzhixing12345/klinux/releases/download/v0.0.1/hdc-0.11.img


```bash
# 制作 Image
make
# 启动
make start
# 调试
make debug
```

## 参考

- [linux tree](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/)
- Linux内核完全注释  
- 深入linux内核架构
