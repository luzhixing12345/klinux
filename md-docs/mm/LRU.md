
# LRU

当内存变得紧张时, 内核必须找到一种方法来释放一些内存页.在某种程度上,内核可以通过清理自己的内部数据结构来释放内存,例如,通过减少 inode 和 dentry 缓存的大小. 但是在大多数系统中,**用户态内存页占用了所消耗的内存量的大头**,毕竟,系统存在的第一要旨就是为应用服务.因此,为了满足当前应用对用户态内存页的分配需求,内核必须找到并释放一些已分配的内存页.

同时系统必须维持文件页和匿名页之间数量上的适当平衡,否则系统将无法正常运行.无论是通过牺牲哪一种内存页为代价而让另一种内存页在数量上占优势,都会导致一种称之为 "thrashing" 的现象.

> [!NOTE]
> 指当系统内存不足时,页框回收算法(Page Frame Reclaiming Algrithom,简称 PFRA),会全力把页框上的内容写入磁盘以便回收这些本属于进程的页框;而同时由于这些进程要继续执行,也会努力申请页框存放(譬如缓存)其内容.因此内核把 PFRA 刚释放的页框又分配给这些进程,并从磁盘读回其内容.其结果就是**数据被无休止地写入磁盘并且再从磁盘读回.大部分的时间耗费在访问磁盘上,而进程则无法实质性地运行下去**.

那如何合理地挑选要回收的 page frames 呢?通常,应该选择那些最近不使用(不活跃)的页面,依据是最近不使用的页面在较短的时间内也不会被频繁使用,这就是 LRU(Least Recently Used) 算法的基本思想.

## 基本思路

为了找到那些最近不活跃的 page ,我们需要有 timestamp 来标识一个 page 最近被访问的时间,然而像 x86 这样的架构并没有从硬件上提供这种机制.

在 x86 中,当一个 page 被访问后,硬件会将 page 对应的 PTE 中的 Access 位置 1,如果完全按照 LRU的思想来设计,那么此时软件需要为这个 page 关联一个定时器,然后将 PTE 中的 Access 位清 0.在定时器 timeout 之前,如果该 page 没有再被访问到,那么就将这个 page 回收.然而,page frames 数量庞大,维护那么多的内核定时器显然是不切实际的.

那如何实现才能保证相对公平又高效呢? Linux 采用的方法是维护 2 个双向链表,一个是包含了最近使用页面的 active list,另一个是包含了最近不使用页面的 inactive list(struct page 中的 lru 域含有指向所在链表中前后页面的指针),并且在 struct page 的 page flags 中使用了 `PG_referenced` 和 `PG_active` 两个标志位来标识页面的活跃程度.

```c{#19,#47}
enum pageflags {
	PG_locked,		/* Page is locked. Don't touch. */
	PG_writeback,		/* Page is under writeback */
	PG_referenced,
	PG_uptodate,
	PG_dirty,
	PG_lru,
	PG_head,		/* Must be in bit 6 */
	PG_waiters,		/* Page has waiters, check its waitqueue. Must be bit #7 and in the same byte as "PG_locked" */
	PG_active,
	PG_workingset,
	PG_reserved,
	PG_reclaim,		/* To be reclaimed asap */
	PG_swapbacked,		/* Page is backed by RAM/swap */
	PG_unevictable		/* Page is "unevictable"  */
    ...
};
```

- `PG_referenced` 标志位表明 page 最近是否被使用过.
- `PG_active` 标志位决定 page 在哪个链表,也就是说 active list 中的 pages 的 PG_active 都为 1,而 inactive list 中的 pages 的 `PG_active` 都为 0.

因此每个 page 都会有四个状态 [active/inactive + referenced/unreferenced]. 不管是 active list 还是 inactive list,都是采用 FIFO(First In First Out) 的形式,新的元素从链表头部加入,中间的元素逐渐向尾端移动.在需要进行内存回收时,内核总是选择 inactive list 尾端的页面进行回收.

对于一个新加入的页其初始状态为 inactive+unreferenced, 每次访问后该页面状态的变化如下图所示

- inactive,unreferenced  ->    inactive,referenced | 第一次access
- inactive,referenced    ->    active,unreferenced | 第二次access
- active,unreferenced    ->    active,referenced   | 第三次access

![20241223232112](https://raw.githubusercontent.com/learner-lu/picbed/master/20241223232112.png)

- 如果 inactive list 上 PG_referenced 为 1 的 page 在回收之前被再次访问到,也就是说它在 inactive list 中时被访问了 2 次,那么就会通过 `activate_page()` 被调整到 active list 的头部,同时将其 PG_active 位置1,PG_referenced 位清 0,这个过程叫做 **promotion**(页面提升), 此时页面从 inactive list 被移动到 active list
- 当 active list 中的一个 page 在到达链表尾端时,如果其 PG_referenced 位为 1,则被放回链表头部,但同时其PG_referenced 会被清 0.如果其 PG_referenced 位为 0,那么就会被放入 inactive list 的头部,这个过程叫做 **demotion**(页面降级), 此时页面由 active list 被移动到 inactive list
- 如果 inactive list 中的 page 在达到链表尾端的时候 PG_referenced 为 0(在 inactive list 期间没有被访问到),那还有什么可说的,不回收你回收谁, 此时页面直接被 reclaim (回收)

![20241223234246](https://raw.githubusercontent.com/learner-lu/picbed/master/20241223234246.png)

## 内核实现

现代内核中 LRU 并不只有 active/inactive 两种, 而是有五种, 分别对应匿名/文件的活跃/不活跃, 以及不可回收的页面

```c
#define LRU_BASE 0
#define LRU_ACTIVE 1
#define LRU_FILE 2

enum lru_list {
	LRU_INACTIVE_ANON = LRU_BASE,
	LRU_ACTIVE_ANON = LRU_BASE + LRU_ACTIVE,
	LRU_INACTIVE_FILE = LRU_BASE + LRU_FILE,
	LRU_ACTIVE_FILE = LRU_BASE + LRU_FILE + LRU_ACTIVE,
	LRU_UNEVICTABLE,
	NR_LRU_LISTS
};
```

每一个 numa node 对应的结构体 pglist_data 中都有一个 `struct lruvec` 保存该节点上的 LRU 链表信息

```c{7}
typedef struct pglist_data {
    ...
    struct lruvec        __lruvec;
} pg_data_t;

struct lruvec {
	struct list_head		lists[NR_LRU_LISTS];
	/* per lruvec lru_lock for memcg */
	spinlock_t			lru_lock;
	/*
	 * These track the cost of reclaiming one LRU - file or anon -
	 * over the other. As the observed cost of reclaiming one LRU
	 * increases, the reclaim scan balance tips toward the other.
	 */
	unsigned long			anon_cost;
	unsigned long			file_cost;
	/* Non-resident age, driven by LRU movement */
	atomic_long_t			nonresident_age;
	/* Refaults at the time of last reclaim cycle */
	unsigned long			refaults[ANON_AND_FILE];
	/* Various lruvec state flags (enum lruvec_flags) */
	unsigned long			flags;
#ifdef CONFIG_LRU_GEN
	/* evictable pages divided into generations */
	struct lru_gen_folio		lrugen;
	/* to concurrently iterate lru_gen_mm_list */
	struct lru_gen_mm_state		mm_state;
#endif
#ifdef CONFIG_MEMCG
	struct pglist_data *pgdat;
#endif
};
```

### 加入 lru

自然我们很关注的一个问题是 page 是什么时候加入到lru链表中的呢? lru链表是页面回收用的,所以只有分配了的页面才有可能存在于lru链中.那很有可能在分配页面的时候把它加进去.事实的确如此

```c
static vm_fault_t do_anonymous_page(struct vm_fault *vmf)
{
    ...
    folio_add_lru_vma(folio, vma);
    ...
}
```

> 还有很多地方也会使用 folio_add_lru 将新增页面加入 lru, 这里只是举一个例子
>
> 有关 folio 的介绍详见 [folio](./folio.md)

前文我们提到每个 node 保存一个 lruvec, 意味着该lru链是该节点全局的,加入lru链需要持有相关的锁,可想而知当分配或回收密集的时候**锁的争用非常严重**. 此外 inactive list 中尾端的页面不断被回收,相当于一个消费者,active list 则不断地将尾端 PG_referenced 为 0 的页面放入 inactive list,相当于一个生产者.不难想象,**这 2 个链表的锁(lru_lock)应该是高度竞争的**,如果从active list 向 inactive list 的页面转移是一个一个进行的,那对锁的争抢将会十分严重.

于是内核使用 per-cpu 的 lru cache (struct pagevec)去做优化. 对于要加入lru链的页,可以先加入该 CPU 的 lru cache,直到 lru cache 中已经积累了 PAGEVEC_SIZE(15)个页面,再获取lru_lock,将这些页面批量放入 inactive list 中,这样可以大大减少对lru链的引用.

我们来看一下对应的函数实现. 如下所示, 该函数会获取per cpu lru cache结构并且将该 folio 加入到 fbatch 当中

```c{14-16}
void folio_add_lru(struct folio *folio)
{
    struct folio_batch *fbatch;

    /* see the comment in lru_gen_add_folio() */
    if (lru_gen_enabled() && !folio_test_unevictable(folio) &&
        lru_gen_in_fault() && !(current->flags & PF_MEMALLOC))
            //设置页的active标志
            folio_set_active(folio);
    //增加页refcount计数
    folio_get(folio);
    local_lock(&cpu_fbatches.lock);
    //获取per cpu lru cache结构
    fbatch = this_cpu_ptr(&cpu_fbatches.lru_add);
    //将页面加入per cpu lru cache或者lru_active_anon链
    folio_batch_add_and_move(fbatch, folio, lru_add_fn);
    local_unlock(&cpu_fbatches.lock);
}
```

每个 CPU 都会分配一个对应的结构体, 其中 lru_add 对应的 folio_batch 的最大长度为 `PAGEVEC_SIZE`(15)

```c{3,19}
struct cpu_fbatches {
	local_lock_t lock;
	struct folio_batch lru_add;
	struct folio_batch lru_deactivate_file;
	struct folio_batch lru_deactivate;
	struct folio_batch lru_lazyfree;
#ifdef CONFIG_SMP
	struct folio_batch activate;
#endif
};
static DEFINE_PER_CPU(struct cpu_fbatches, cpu_fbatches) = {
	.lock = INIT_LOCAL_LOCK(lock),
};

#define PAGEVEC_SIZE	15
struct folio_batch {
	unsigned char nr;
	bool percpu_pvec_drained;
	struct folio *folios[PAGEVEC_SIZE];
};
```

可以看到页会先加入fbatch如果没问题就直接返回,否则就要把整个fbatch都加入lru链.

```c
static void folio_batch_add_and_move(struct folio_batch *fbatch,
		struct folio *folio, move_fn_t move_fn)
{
    // fbatch 还有剩余空间, 直接加入
	if (folio_batch_add(fbatch, folio) && !folio_test_large(folio) &&
	    !lru_cache_disabled())
		return;
    // 满了则将整个 fbetch 加入到 node lru
	folio_batch_move_lru(fbatch, move_fn);
}
```

folio_batch_add 非常简单, 就是直接加入数组下一个元素, 直到满(PAGEVEC_SIZE)了返回 0

```c
static inline unsigned folio_batch_add(struct folio_batch *fbatch,
		struct folio *folio)
{
	fbatch->folios[fbatch->nr++] = folio;
	return folio_batch_space(fbatch);
}
static inline unsigned int folio_batch_space(struct folio_batch *fbatch)
{
	return PAGEVEC_SIZE - fbatch->nr;
}
```

如果该 CPU 对应的 fbatch 满了, 此时调用 folio_batch_move_lru 获取 node 的 lruvec 并将 fbatch 中的所有元素逐个加入

```c{14,15, 17}
static void folio_batch_move_lru(struct folio_batch *fbatch, move_fn_t move_fn)
{
	int i;
	struct lruvec *lruvec = NULL;
	unsigned long flags = 0;

	for (i = 0; i < folio_batch_count(fbatch); i++) {
		struct folio *folio = fbatch->folios[i];

		/* block memcg migration while the folio moves between lru */
		if (move_fn != lru_add_fn && !folio_test_clear_lru(folio))
			continue;

		lruvec = folio_lruvec_relock_irqsave(folio, lruvec, &flags);
		move_fn(lruvec, folio);

		folio_set_lru(folio);
	}

	if (lruvec)
		unlock_page_lruvec_irqrestore(lruvec, flags);
	folios_put(fbatch->folios, folio_batch_count(fbatch));
	folio_batch_reinit(fbatch);
}
```

folio_lruvec_relock_irqsave 层层往下调用最终获取到的 lruvec 实际上就是 node lruvec

> 这里面还有 mem cgroup 的参与, 代码阅读起来有点绕, 简单来说就是如果 disable memcgroup 那么返回的就是 pgdat 中的 __lruvec 链表
> ```c{8}
> static inline struct lruvec *mem_cgroup_lruvec(struct mem_cgroup *memcg,
> 					       struct pglist_data *pgdat)
> {
> 	struct mem_cgroup_per_node *mz;
> 	struct lruvec *lruvec;
> 
> 	if (mem_cgroup_disabled()) {
> 		lruvec = &pgdat->__lruvec;
> 		goto out;
> 	}
> 	mz = memcg->nodeinfo[pgdat->node_id];
> 	lruvec = &mz->lruvec;
> 	return lruvec;
> }
> ```

move_fn 则负责将一个 folio 加入到 lru 中, 通过往前回溯可以发现该函数指针在 folio_add_lru 中调用时的传参为 lru_add_fn. 该函数前面是对于 evictable/active 标志位的检查和清空, 关键在于最后的 lruvec_add_folio

```c{17}
static void lru_add_fn(struct lruvec *lruvec, struct folio *folio)
{
	int was_unevictable = folio_test_clear_unevictable(folio);
	long nr_pages = folio_nr_pages(folio);

	if (folio_evictable(folio)) {
		if (was_unevictable)
			__count_vm_events(UNEVICTABLE_PGRESCUED, nr_pages);
	} else {
		folio_clear_active(folio);
		folio_set_unevictable(folio);
		folio->mlock_count = 0;
		if (!was_unevictable)
			__count_vm_events(UNEVICTABLE_PGCULLED, nr_pages);
	}

	lruvec_add_folio(lruvec, folio);
	trace_mm_lru_insertion(folio);
}
```

lruvec_add_folio 首先判断一下该 folio 属于哪种 list, 然后调用 list_add 最终将其加入到 lruvec 对应的 lists[lru] 中

```c{13}
static __always_inline
void lruvec_add_folio(struct lruvec *lruvec, struct folio *folio)
{
    // 判断该 folio 属于哪个 list, file/anon
	enum lru_list lru = folio_lru_list(folio);

	if (lru_gen_add_folio(lruvec, folio, false))
		return;

	update_lru_size(lruvec, lru, folio_zonenum(folio),
			folio_nr_pages(folio));
	if (lru != LRU_UNEVICTABLE)
		list_add(&folio->lru, &lruvec->lists[lru]);
}
```

> [!TIP]
> folio->lru 和 lruvec->lists[lru] 都是 list_head 类型, 它们本身并不保存任何相关页面信息, 只是作为指针地址将二者关联起来
>
> 即可以通过 folio->lru 找到 lruvec->list[lru] 也可以通过遍历 lruvec->list[lru] 找到 folio
>
> 只是通过指针建立了双向链表的地址引用, 如果希望删掉 folio 直接 list_del 即可

folio_lru_list 的注释也写的很清晰, 先判断 file/anon, 然后再判断是否 active

```c
/**
 * folio_lru_list - Which LRU list should a folio be on?
 * @folio: The folio to test.
 *
 * Return: The LRU list a folio should be on, as an index
 * into the array of LRU lists.
 */
static __always_inline enum lru_list folio_lru_list(struct folio *folio)
{
	enum lru_list lru;

	VM_BUG_ON_FOLIO(folio_test_active(folio) && folio_test_unevictable(folio), folio);

	if (folio_test_unevictable(folio))
		return LRU_UNEVICTABLE;

	lru = folio_is_file_lru(folio) ? LRU_INACTIVE_FILE : LRU_INACTIVE_ANON;
	if (folio_test_active(folio))
		lru += LRU_ACTIVE;

	return lru;
}
```

> [!NOTE]
> lru += LRU_ACTIVE 这个操作和前文 enum lru_list 的定义相关, 算是一种枚举值的巧用, 不然判断估计要写好几个 if else

如果没有显示的设置active那就会将page放到inactive链表中, 这也是默认情况. 一般新创建的内存页都会先加入 inactive list 中.

---

至此我们梳理一下整个过程, 关键函数调用如下

```txt
do_anonymous_page-----------------------------分配匿名页面
    folio_add_lru-----------------------------加入到 lru list
        folio_batch_add_and_move--------------批量加入到 cpu fbatch 中
            - folio_batch_add-----------------优先加入到 per-cpu lru cache
            folio_batch_move_lru--------------满了则将 fbatch 加入 node lruvec, 设置 PG_lru
                folio_lruvec_relock_irqsave---获取 node lruvec
                lru_add_fn(lruvec, folio);
                    lruvec_add_folio
                        folio_lru_list--------判断folio类型
                        list_add--------------加入lruvec
```

> [!IMPORTANT]
> 需要注意的是, 为 folio 添加 PG_lru 标志位发生在 folio_batch_move_lru 中, 表示确认将其加入到 lru list 中了.
>
> folio_batch_add 并不会对 folio 设置此标记位, 因此尚在 per-cpu lru cache 的 folio 并不算在 lru list 中


### 页面访问

在分配匿名页的时候会将分配的页放在该folio所在node的lruvec的对应链表里,从代码中看到第一次加入lru链表是在inactive链表中

如果页面一直呆在inactive链表中是很"危险"的, 因为随着后续页面的不断加入, 该页面会不断地从 list 的头部被推向尾部, 到达 inactive list 尾部时就会被回收.如果不想被回收那该怎么做呢?这就是页面在lru链表上如何移动的问题.与此相关的页的flag有PG_referenced和PG_active.

假设初始状态两个标志都是0,他们的状态变化如下

- inactive,unreferenced  ->    inactive,referenced | 第一次access
- inactive,referenced    ->    active,unreferenced | 第二次access
- active,unreferenced    ->    active,referenced   | 第三次access

![20241223232112](https://raw.githubusercontent.com/learner-lu/picbed/master/20241223232112.png)

对于 mmap 的页面,需要在回收扫描时,通过 rmap 检查 PTE 的 Access 位来判断,而对于通过页面在文件的 offset 来作为索引的(比如 read/write),则可以在 page 被访问后标记该信息. 例如 filemap_read 函数中, 实现此方向状态转移的函数是 folio_mark_accessed

```c
ssize_t filemap_read(struct kiocb *iocb, struct iov_iter *iter,
		ssize_t already_read) 
{
    folio_mark_accessed(folio);
}
```

folio_mark_accessed 用于标记一个 folio 最近被访问过了, 这里的几个 if else if 很巧妙;

- 如果 unreferenced, 设置 referenced = 1
- 否则此时一定 referenced = 1, 如果 inactive
  - 如果已经在 lru list 中, 标记为 active
  - 不在 lru 中, 还在 lru cache 中, 找到并标记 active
- referenced && active 的不需要处理

```c{15-18}
void folio_mark_accessed(struct folio *folio)
{
	if (lru_gen_enabled()) {
		folio_inc_refs(folio);
		return;
	}

    // 如果 referenced = 0, 设置 referenced = 1
	if (!folio_test_referenced(folio)) {
		folio_set_referenced(folio);
	} else if (folio_test_unevictable(folio)) {
        // 不可回收页面不需要处理
	} else if (!folio_test_active(folio)) {
        // 此时 referenced 一定为 1, 清除 referenced 并设置 active
		if (folio_test_lru(folio))
			folio_activate(folio);
		else
			__lru_cache_activate_folio(folio);
		folio_clear_referenced(folio);
		workingset_activation(folio);
	}
    // referened 且 active 的不需要处理

	if (folio_test_idle(folio))
		folio_clear_idle(folio);
}
```

如果 folio 已经在 lru 中, 则调用 folio_activate. 该函数与前文提到的 folio_add_lru 基本类似, 也是使用 folio_batch_add_and_move. 不同点在于函数指针为 folio_activate_fn

```c
void folio_activate(struct folio *folio)
{
	if (folio_test_lru(folio) && !folio_test_active(folio) &&
	    !folio_test_unevictable(folio)) {
		struct folio_batch *fbatch;

		folio_get(folio);
		local_lock(&cpu_fbatches.lock);
		fbatch = this_cpu_ptr(&cpu_fbatches.activate);
		folio_batch_add_and_move(fbatch, folio, folio_activate_fn);
		local_unlock(&cpu_fbatches.lock);
	}
}
```

和前文处理逻辑类似, 在 folio_batch_add_and_move 中如果 fbatch 已满, 则将其批量移入 node lru, 并调用 move_fn. 此时需要执行的操作即

1. 从 lru 中删除当前 folio
2. 将 folio 设置为 active
3. 把 folio 重新添加回 lru 中, 使其位于 active list 头部

```c{6-8}
static void folio_activate_fn(struct lruvec *lruvec, struct folio *folio)
{
	if (!folio_test_active(folio) && !folio_test_unevictable(folio)) {
		long nr_pages = folio_nr_pages(folio);

		lruvec_del_folio(lruvec, folio);
		folio_set_active(folio);
		lruvec_add_folio(lruvec, folio);
		trace_mm_lru_activate(folio);

		__count_vm_events(PGACTIVATE, nr_pages);
		__count_memcg_events(lruvec_memcg(lruvec), PGACTIVATE,
				     nr_pages);
	}
}
```

而对于另一种情况, 即 folio 不在 lru 中, 那么就去 cpu lru cache 中查找到对应的 folio 并将其设置为 active

> 这里的一个小细节是遍历时从最后一个开始查找, 与向 folio_batch 添加 folio 时刚好相反

```c
static void __lru_cache_activate_folio(struct folio *folio)
{
	struct folio_batch *fbatch;
	int i;

	local_lock(&cpu_fbatches.lock);
	fbatch = this_cpu_ptr(&cpu_fbatches.lru_add);

	for (i = folio_batch_count(fbatch) - 1; i >= 0; i--) {
		struct folio *batch_folio = fbatch->folios[i];

		if (batch_folio == folio) {
			folio_set_active(folio);
			break;
		}
	}

	local_unlock(&cpu_fbatches.lock);
}
```

## 一些细节

TODO list 长度?

内核提供了一个名为 swappiness 的控制项(`/proc/sys/vm/swappiness`, 默认60),可以调节这种平衡.如果系统管理员设置的 swappiness 值较高,则意味着内核允许在内存中保留较多的 cache page.将 swappiness 调低则是告诉内核回收更多的 cache page,保留较多的 anonymous page.


## 参考

- [linux内存源码分析 - 内存回收(lru链表)](https://www.cnblogs.com/tolimit/p/5447448.html)
- [linux 内存回收lru算法代码注释1](https://blog.csdn.net/qq_37517281/article/details/134533485)
- [Linux中的内存回收 [一]](https://zhuanlan.zhihu.com/p/70964195)
- [linux内存管理(九)- 页面回收](https://www.cnblogs.com/banshanjushi/p/18003021)
- [内存管理](https://blog.csdn.net/u012489236/category_9614673.html)