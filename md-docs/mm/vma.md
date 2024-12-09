
# vma

即使人们常说一个进程"被赋予"了一个唯一的4GB虚拟地址空间,但这并不意味着进程可以在该空间中为所欲为:要访问该虚拟空间内的Paricular区域,它必须要求内核为其提供所谓的"有效"内存区域 - Linux中定义的相应数据结构称为"vm_area_struct"或 VMA.您可以通过进程 ID 上的"pmap"命令检查与进程关联的所有 VMA.

## 参考

- [【原创】(十三)Linux内存管理之vma/malloc/mmap](https://www.cnblogs.com/LoyenWang/p/12037658.html)