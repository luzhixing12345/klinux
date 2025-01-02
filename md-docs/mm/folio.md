
# folio

> [!NOTE]
> There are only two hard things in Computer Science: cache invalidation and naming things.
> 
> 计算机科学中只有两件难事:缓存失效和命名.
> 
> -- Phil Karlton

取名,要用名字恰到好处地描述其想描述的事物,要体现代码注释的最高原则: 源码即注释,这其实一点都不轻松.取名一般都是从生僻的变为大众的,这样才能朗朗上口,为人民群众所喜闻乐见,比如陈港生更名为成龙,杨旎奥改名为杨紫,刘福荣改名为刘德华.而内核从page到folio的一次改变,似乎是反其道而行之了.感觉有相当数量的同学可能都不见得认识folio这个单词.

folio 的英文解释是 **由一张或多张整张纸组成的书籍或小册子**, 和单页(page)对应. 它的目标在于解决内核面临的一个纠结状况.至于这个名字叫folio、pageset、superpage还是head_page,其实都没有那么重要了,背后真正重要的是,它要解决什么问题

## 乱局

众所周知,在Linux内核中,我们通常使用page来描述一页,这一页通常是4KB.这个世界如果所有人都是4KB的单页,那就简单归一了.但是,在晴朗的天空中,却漂浮中一朵乌云,这朵乌云就是compound page以及由compound page衍生出的hugepage,它们并非总是单页的.

在Linux中,我们并不总是以单一的4KB basepage为单位来获取、映射和释放内存.我们有时候,会把多个4KB复合在一起,进行申请、映射和释放:



在 Linux 内核中,`struct folio` 和 `struct page` 都是用于描述物理内存页面的结构体,但它们的设计目标和使用场景有所不同.

---

### **1. `struct page`**
- `struct page` 是内核传统的基础内存管理结构体,用于描述单个物理内存页.
- 每个物理页帧(通常为 4 KB)都有一个与之对应的 `struct page`.
- 在大页(如 HugePage 或 THP,Transparent HugePage)的场景下,每个物理页帧仍可能有一个 `struct page`,但多个连续的 `struct page` 会被联合管理.

---

### **2. `struct folio`**
- `struct folio` 是 Linux 内核中一种更现代的内存管理结构,旨在优化和简化管理大页面的逻辑.
- `folio` 是对一组物理连续页面的抽象,代表最小内存分配和操作单位.
- 它主要用于减少 `struct page` 的开销和复杂性,因为在大页场景中,操作单个页面的概念可能过于低效.

**引入的原因**:
- 使用 `struct page` 时,大页面需要跨多个 `struct page` 的边界管理,逻辑复杂且效率低下.
- `folio` 提供了一个更一致的抽象,允许对一组连续页面进行操作,而不是单个页面.

---

### **3. `folio` 和 `page` 的关系**
- **互相映射**:
  - `folio` 包含一个或多个 `struct page`.
  - 一个 `struct page` 总是属于一个 `folio`.

- **互相转换**:
  - 从 `struct page` 获取 `folio`:
    ```c
    struct folio *folio = page_folio(page);
    ```
  - 从 `struct folio` 获取第一个 `struct page`:
    ```c
    struct page *page = folio_page(folio, 0); // 获取第一个页面
    ```
  - 访问其他 `struct page`:
    ```c
    struct page *page_n = folio_page(folio, n); // 获取第 n 个页面
    ```

- **数据布局**:
  - `folio` 本质上是一种"增强型"的 `page`,其数据布局在内存中共享,因此 `folio` 和 `page` 可以通过指针类型安全地互相转换.

---

### **4. `folio` 的优势**
- **性能优化**:
  - 减少对多个 `struct page` 的遍历,提高对大页(如 THP)操作的效率.
  - 优化内存分配和管理逻辑.
  
- **逻辑简化**:
  - 对于通常以大块分配的内存(如文件缓存、匿名内存等),`folio` 提供了更直观的操作接口.

- **减少内存开销**:
  - 对于大页场景,`folio` 减少了 `struct page` 的重复使用,提高了内核的内存效率.

---

### **5. 示例代码**
以下是如何从 `folio` 和 `page` 互相转换的简单代码:

```c
#include <linux/mm.h>
#include <linux/folio.h>

// 获取 folio 的信息
void print_folio_info(struct page *page)
{
    struct folio *folio = page_folio(page);

    printk(KERN_INFO "Folio address: %p\n", folio);
    printk(KERN_INFO "Folio order (size): %d\n", folio_order(folio));
    printk(KERN_INFO "Folio refcount: %d\n", folio_ref_count(folio));

    // 获取 folio 内的第一个 page
    struct page *folio_start_page = folio_page(folio, 0);
    printk(KERN_INFO "Folio's first page address: %p\n", folio_start_page);
}

// 检查 folio 是否匿名
bool is_folio_anon(struct page *page)
{
    struct folio *folio = page_folio(page);
    return folio_test_anon(folio);
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

---

### **6. 总结**
- **`struct page`**:用于描述单个物理页面,是更基础的内存管理单位.
- **`struct folio`**:用于描述一组连续页面,适合管理大页面和文件缓存等更高效的场景.
- `folio` 是对 `page` 的增强,其引入是为了适应现代内存管理需求,减少 `page` 在大块内存管理中的复杂性和性能瓶颈.

在新版本的 Linux 内核中,`folio` 的使用逐渐替代了大部分涉及大页的 `page` 操作.

## folio 相关函数

代码中我们可能经常会看到 folio_test_lru, folio_set_lru 等函数, 但是好像又没有直接看到它的定义. 通过 clangd 我们可以追溯到该宏定义 PAGEFLAG, 其定义了相关的函数展开

```c
#define PAGEFLAG(uname, lname, policy)					\
	TESTPAGEFLAG(uname, lname, policy)				\
	SETPAGEFLAG(uname, lname, policy)				\
	CLEARPAGEFLAG(uname, lname, policy)

PAGEFLAG(Referenced, referenced, PF_HEAD)
	TESTCLEARFLAG(Referenced, referenced, PF_HEAD)
	__SETPAGEFLAG(Referenced, referenced, PF_HEAD)
PAGEFLAG(Dirty, dirty, PF_HEAD) TESTSCFLAG(Dirty, dirty, PF_HEAD)
	__CLEARPAGEFLAG(Dirty, dirty, PF_HEAD)
PAGEFLAG(LRU, lru, PF_HEAD) __CLEARPAGEFLAG(LRU, lru, PF_HEAD)
	TESTCLEARFLAG(LRU, lru, PF_HEAD)
PAGEFLAG(Active, active, PF_HEAD) __CLEARPAGEFLAG(Active, active, PF_HEAD)
	TESTCLEARFLAG(Active, active, PF_HEAD)
```

以 TESTPAGEFLAG 为例, 其会被展开为 folio_test_lru 和 Pagelru 函数. 函数实现其实就是对于该 bit 的判断

```c
#define TESTPAGEFLAG(uname, lname, policy)				\
static __always_inline bool folio_test_##lname(struct folio *folio)	\
{ return test_bit(PG_##lname, folio_flags(folio, FOLIO_##policy)); }	\
static __always_inline int Page##uname(struct page *page)		\
{ return test_bit(PG_##lname, &policy(page, 0)->flags); }
```

简而言之, `PAGEFLAG(Referenced, referenced, PF_HEAD)` 实际上就相当于定义了一大串的

```c
static __always_inline bool folio_test_referenced(struct folio *folio)
{
	return test_bit(PG_referenced, folio_flags(folio, FOLIO_PF_HEAD));
}
static __always_inline int PageReferenced(struct page *page)
{
	return test_bit(PG_referenced, &PF_HEAD(page, 0)->flags);
}
static __always_inline void folio_set_referenced(struct folio *folio)
{
	set_bit(PG_referenced, folio_flags(folio, FOLIO_PF_HEAD));
}
static __always_inline void SetPageReferenced(struct page *page)
{
	set_bit(PG_referenced, &PF_HEAD(page, 1)->flags);
}
static __always_inline void folio_clear_referenced(struct folio *folio)
{
	clear_bit(PG_referenced, folio_flags(folio, FOLIO_PF_HEAD));
}
static __always_inline void ClearPageReferenced(struct page *page)
{
	clear_bit(PG_referenced, &PF_HEAD(page, 1)->flags);
}
```

> [!TIP]
> 看不清的话可以把 include/linux/page-flags.h 这部分宏定义 copy 下来然后用预处理器展开一下
>
> ```bash
> gcc -E a.c -i a.i
> ```

## 参考

- [论好名字的重要性: Linux内核page到folio的变迁](https://blog.csdn.net/feelabclihu/article/details/131485936)