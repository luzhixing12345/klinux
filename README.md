# klinux

klinux 是一个精简的现代linux kernel, 在Linux 0.11的基础上加入了诸多现代 linux kernel的优秀新特性,本项目意在

- 维护一个**易于学习的**, **紧跟新特性的**, **可编译运行使用的** 最小linux内核
- 实现现代linux kernel中操作系统的新功能, 精简去除了大部分驱动和我认为不重要的代码,文档
- 持续关注内核 `内存` `进程` `网络` `驱动` `文件系统` 相关进展, 与最新版本有用的功能保持同步

## 添加内容包括

- [ ] Linux 1.0 (1994年3月): TCP/IP网络协议、系统调用接口、多处理器支持
- [ ] Linux 2.0 (1996年6月): Symmetric Multiprocessing (SMP)、PCI总线和支持更多设备的扩展性
- [ ] Linux 2.2 (1999年1月): 对USB、PCMCIA、AGP和IPSec的支持
- [ ] Linux 2.4 (2001年1月): 对更多硬件、网络协议和文件系统的支持，以及更好的SMP支持和内存管理
- [ ] Linux 2.6 (2003年12月): 内核线程、内核调试器、处理器调度器、NUMA、基于硬件的虚拟化等
- [ ] Linux 3.x (2011年5月): Cgroup、Btrfs、内核崩溃转储、网络命名空间等
- [ ] Linux 4.x (2015年4月): 对Intel Skylake处理器、新的文件系统支持（如DAX、F2FS等）、安全改进（如内核密钥环等）等

## 兼容性(其他架构暂不考虑)

- [ ] X86
- [ ] ARM
- [ ] RISC-V

## 文档

https://luzhixing12345.github.io/klinux

## 参考

- 深入linux内核架构