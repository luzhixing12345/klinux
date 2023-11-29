
# linux目录结构

一级目录结构如下, 笔者个人的关注重点是 `内存管理 mm`

```bash
.
├── Documentation              # 包含Linux内核的文档
├── LICENSES                   # 包含Linux内核的许可证文件
├── arch                       # 与不同体系结构相关的代码和配置文件
├── block                      # 与块设备和存储子系统相关的代码
├── certs                      # 用于验证模块和驱动程序签名的数字证书
├── crypto                     # Linux内核的加密相关代码和算法
├── drivers                    # 各种设备驱动程序
├── fs                         # 文件系统相关的代码和实现
├── include                    # 内核编程的头文件
├── init                       # 内核初始化相关的代码和配置文件
├── io_uring                   # Linux内核的异步I/O框架相关的代码
├── ipc                        # 进程间通信(IPC)机制的实现
├── kernel                     # 核心内核代码
├── lib                        # 与内核相关的通用库函数和工具函数
├── mm                         # 内存管理相关的代码
├── net                        # 网络协议栈和网络驱动程序相关的代码
├── rust                       # 使用Rust编写的内核模块和驱动程序
├── samples                    # 示例代码
├── scripts                    # 构建和配置内核的脚本和工具
├── security                   # 与系统安全性相关的代码和模块
├── sound                      # 声音子系统相关的代码和驱动程序
├── tools                      # 开发和调试Linux内核的工具和实用程序
├── usr                        # 用户空间工具和库文件
└── virt                       # 虚拟化相关的代码和驱动程序
```

## 参考

- [LINUX内核目录文件说明以及配置并编译内核的方法](https://blog.csdn.net/ffmxnjm/article/details/72933915)