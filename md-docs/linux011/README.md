
# README

## Linux0.11

早期的Linux代码被保存在 <http://oldlinux.org/> 上, 其中还保存着一些诸如 [VMware-images](http://www.oldlinux.org/Linux.old/VMware-images/) 和 [qemu-images](http://www.oldlinux.org/Linux.old/qemu-images/)

源码下载只需要找到其中的0.11版本的即可

```bash
wget http://www.oldlinux.org/Linux.old/Linux-0.11/sources/system/linux-0.11.tar.Z
tar xf linux-0.11.tar.Z
```

笔者也将代码上传了一份用于存档, 下面讨论的 Linux0.11 版本的代码均基于此: [Linux0.11源码](https://github.com/luzhixing12345/klinux/releases/download/v0.0.1/linux-0.11.tar.Z)

解压之后的文件夹比较清晰, 各部分的功能简要概述如下所示

```bash
├── boot          # 系统引导汇编程序
├── fs            # 文件系统
├── include       # 头文件
│   ├── asm       # 与CPU体系结构相关的部分
│   ├── linux     # Linux内核专用部分
│   └── sys       # 系统数据结构部分
├── init          # 内核初始化程序
├── kernel        # 内核进程调度 信号处理 系统调用
│   ├── blk_drv   # 块设备驱动程序
│   ├── chr_drv   # 字符设备驱动程序
│   └── math      # 数学协处理器仿真处理程序
├── lib           # 内核库函数
├── mm            # 内存管理程序
└── tools         # 生成内核Image文件的工具程序
```

## 开发环境

笔者使用的是 WSL2 + Vscode + clangd

```json
{
    "clangd.fallbackFlags": [
        "-I${workspaceFolder}/include"
    ],
    "clangd.arguments": [
        "--background-index", // 在后台自动分析文件（基于complie_commands)
        "-j=12", // 同时开启的任务数量
        "--clang-tidy", // clang-tidy功能
        "--clang-tidy-checks=performance-*,bugprone-*",
        "--all-scopes-completion", // 全局补全（会自动补充头文件）
        "--completion-style=detailed", // 更详细的补全内容
        "--header-insertion=iwyu" // 补充头文件的形式
    ]
}
```

值得一提的是这里需要使用 `-I${workspaceFolder}/include` 因为默认会优先找 /usr/include 下的 linux 系统头文件

## 编译运行

初代 Linux0.11 源代码并不能直接编译使用, 使用到了很多过于古早的 8086 汇编器和链接器, 这里需要修改一些代码然后使用 QEMU 模拟器进行模拟

```bash
gtk initialization failed
```

![20230604212250](https://raw.githubusercontent.com/learner-lu/picbed/master/20230604212250.png)

![20230604113037](https://raw.githubusercontent.com/learner-lu/picbed/master/20230604113037.png)

## 参考

- [linux-0.11 研究](https://blog.csdn.net/lyndon_li/article/details/130374261)
- [Linux 0.11-调试 Linux 最早期的代码-36](https://blog.csdn.net/m0_53157173/article/details/127786381)
- [Linux0.11 即开即用 Linux/MacOS](https://github.com/yuan-xy/Linux-0.11)
- [ubuntu编译linux0.11](https://blog.csdn.net/wyyy2088511/article/details/111239657)
- [编译linux-0.11的第二种方法](https://blog.csdn.net/wyyy2088511/article/details/111300826)