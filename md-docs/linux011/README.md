
# README

这部分内容主要参考赵炯博士的 [Linux内核完全注释V5](http://oldlinux.org/download/CLK-5.0-WithCover.pdf) 和 [Linux内核完全注释V3](http://www.oldlinux.org/download/clk011c-3.0.pdf), 最新版已更新至 linux0.12 版本

> 个人感觉 v3 版本的顺序要更好一些, v5 的一些图不错

Linux 0.11 已经过时了, 它不支持多用户，甚至没有关机或重启命令, 它也非常不稳定，很容易出现恐慌和崩溃, 不支持 TCP/IP、UUCP、共享库等. 也没有虚拟文件系统 VFS、ext2 或 ext3 文件系统、网络子系统、新的复杂的内存管理机制.

但现代 Linux 的代码量庞大的惊人, 这些版本进行完全注释和说明几乎不可能, 而 0.11 版内核不超过 2 万行代码量，因此完全可以在一本书中解释和注释清楚。麻雀虽小，五脏俱全

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

笔者使用的是 WSL2(Ubuntu22.04) + Vscode + clangd

```json
{
    "clangd.fallbackFlags": [
        "-I${workspaceFolder}/include",
        "-Wno-implicit-function-declaration",
        "-Wno-incompatible-library-redeclaration"
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

如果读者希望快速体验一下, 您可以在这里下载 [Linux 0.11 on qemu-12.5.i386.zip](http://www.oldlinux.org/Linux.old/qemu-images/Linux%200.11%20on%20qemu-12.5.i386.zip), 这是一个很完整的模拟环境(Windows) , 解压之后双击 linux.bat 即可运行, 结果如下

> <kbd>ctrl</kbd> + <kbd>alt</kbd> 退出

![qq5234](https://raw.githubusercontent.com/learner-lu/picbed/master/qq5234.gif)

如果希望从源码编译运行则需要修改很多地方, 读者可参考[Linux0.11 即开即用 Linux/MacOS](https://github.com/yuan-xy/Linux-0.11)的代码, 它可以很方便的直接使用

```bash
sudo apt install qemu qemu-system qemu-kvm build-essential
wget https://github.com/luzhixing12345/klinux/releases/download/v0.0.1/hdc-0.11.img

# 制作 Image
make
# 启动
make start
# 调试
make debug
```

运行结果如下

![20230604232145](https://raw.githubusercontent.com/learner-lu/picbed/master/20230604232145.png)

WSL2 中由于没有图形化界面缺少 GTK 所以稍微有点麻烦, 所以在 Makefile 中的 QEMU_FLAG 使用了 `-nographic` 禁用了 QEMU 的图形化界面, 不然会出现一个 `gtk initialization failed` 的错误, 我个人没有期望 GUI 界面, 更希望使用纯命令行交互模式调试和使用内核, 但是直接运行发现显示很奇怪

![20230604212250](https://raw.githubusercontent.com/learner-lu/picbed/master/20230604212250.png)

如果是直接把 Image 拿到 VMware 中结果如下

![20230619024344](https://raw.githubusercontent.com/learner-lu/picbed/master/20230619024344.png)

> 暂时没有搞定, 总是先看看代码吧...

## 参考

- [linux-0.11 研究](https://blog.csdn.net/lyndon_li/article/details/130374261)
- [Linux 0.11-调试 Linux 最早期的代码-36](https://blog.csdn.net/m0_53157173/article/details/127786381)
- [Linux0.11 即开即用 Linux/MacOS](https://github.com/yuan-xy/Linux-0.11)
- [ubuntu编译linux0.11](https://blog.csdn.net/wyyy2088511/article/details/111239657)
- [编译linux-0.11的第二种方法](https://blog.csdn.net/wyyy2088511/article/details/111300826)
- [如何使用qemu在终端上运行非gui操作系统？](https://stackoverflow.com/questions/6710555/how-to-use-qemu-to-run-a-non-gui-os-on-the-terminal)
- [将 Qemu 控制台重定向到文件或主机终端？](https://stackoverflow.com/questions/18098455/redirect-qemu-console-to-a-file-or-the-host-terminal)