
# page

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