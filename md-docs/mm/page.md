
# page

## 内存页类型

文件页
通过free看到的缓存cache统计对应的缓存页都是文件页(File-backed Page),它们都对应着系统中的文件、数据.如果没有与之对应的文件,我们就称其为匿名页.File-backed Pages在内存不足的时候可以直接写回对应的硬盘文件里,即Page-out,以释放内存,需要时从磁盘再次读取数据.比如我们可以通过echo 3 > /proc/sys/vm/drop_caches的方式释放大部分cache.

匿名页
应用程序使用的堆,栈,数据段等,没有文件背景的页面被称为匿名页(Anonymous Page),它们不是以文件形式存在,因此无法和磁盘文件交换,但可以通过硬盘上划分额外的swap交换分区或使用交换文件进行交换,即Swap-out.匿名页与用户进程共存,进程退出则匿名页释放,而Page Cache即使在进程退出后还可以缓存

脏页
被应用程序修改过,并且暂时还没写入磁盘的数据使用的内存页被称为脏页(Dirty Page).如果要释放这些页面,就得先写入磁盘.这些脏页,一般可以通过两种方式写入磁盘.一个是通过系统调用fsync,把脏页刷到磁盘中;也可以交给系统,由内核线程Pdflush将脏页刷到磁盘.

大页
为了降低TLB miss的概率,Linux引入了Hugepages机制,可以设定Page大小为2MB或者1GB.2MB的Hugepages机制下,同样256GB内存需要的页表项降低为256GB/2MB=131072,仅需要2MB.因此Hugepages的页表可以全量缓存在CPU cache中. 通过sysctl -w vm.nr_hugepages=1024可以设置hugepages的个数为1024,总大小为4GB.需要注意是,设置huagepages会从系统申请连续2MB的内存块并进行保留(不能用于正常内存申请),如果系统运行一段时间导致内存碎片较多时,再申请hugepages会失败.

透明大页
由于Huge pages很难手动管理,而且通常需要对代码进行重大的更改才能有效的使用,因此又引入了Transparent Huge Pages(THP),THP 是一个抽象层,能够自动创建、管理和使用传统大页.标准大页管理是预分配的方式,而透明大页管理则是动态分配的方式.

![20241127233707](https://raw.githubusercontent.com/learner-lu/picbed/master/20241127233707.png)

```c
static void smaps_page_accumulate(struct mem_size_stats *mss,
		struct page *page, unsigned long size, unsigned long pss,
		bool dirty, bool locked, bool private)
{
	mss->pss += pss;

	if (PageAnon(page))
		mss->pss_anon += pss;
	else if (PageSwapBacked(page))
		mss->pss_shmem += pss;
	else
		mss->pss_file += pss;
}
```

```c
int get_page_type(pte_t *pte) {
    struct folio *folio = pfn_folio(pte_pfn(*pte));
    enum page_type page_type;
    if (unlikely(folio_test_ksm(folio))) {
        page_type = UNKNOWN_PAGE;
    } else if (folio_test_anon(folio)) {
        page_type = ANON_PAGE;
    } else {
        page_type = FILE_PAGE;
    }
    return page_type;
}
```

## 参考

- [深入理解 Page Cache](https://cloud.tencent.com/developer/article/2363233) 好
- [Linux内存管理:反向映射机制(匿名页,文件页和ksm页)](https://blog.csdn.net/Rong_Toa/article/details/109864063)
- [cat /proc/meminfo 各字段详解](https://blog.csdn.net/JustDoIt_201603/article/details/106629059)
- [内存页类型](https://blog.csdn.net/u010039418/article/details/103446899)