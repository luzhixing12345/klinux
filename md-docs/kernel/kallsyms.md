
# kallsyms

在内核中维护者一张符号表，记录了内核中所有的符号（函数、全局变量等）的地址以及名字（非栈变量），这个符号表（.tmp_vmlinux2.o）被嵌入到内核镜像中，使得内核可以在运行过程中随时获得一个符号地址对应的符号名。而内核代码中可以通过调用 __print_symbol(const char *fmt, unsigned long address)打印符号名。

## 参考

- [/proc/kallsyms 符号表说明](https://blog.csdn.net/qq_42931917/article/details/129943916)
- [linux内核$(kallsyms.o)详解续篇 --- 内核符号表的生成和查找过程](https://zhuanlan.zhihu.com/p/607285952)
- [access linux kernel symbols that are not exported via export symbol](https://stackoverflow.com/questions/9951516/access-linux-kernel-symbols-that-are-not-exported-via-export-symbol)