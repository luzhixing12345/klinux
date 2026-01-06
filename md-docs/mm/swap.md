
# kswapd

kswapd 是 Linux 内核中的一个内核线程,专门负责内存管理中的页面回收操作。它的主要任务是维护系统内存的平稳运行,通过在内存压力较大时回收不常用的页面,避免系统耗尽内存。

Linux 内核使用分页内存管理,将内存划分为多个页面。当系统中的可用内存低于一定阈值时,需要回收一些页面以供新分配使用。kswapd 是负责这项工作的关键机制。

kswapd 的核心任务是:

- **页面回收**:回收内存中不活跃或未使用的页面(如文件缓存页面),以释放内存。
- **避免内存耗尽**:在内存压力出现之前主动进行回收,确保有足够的空闲页面可供分配。

## kswap 内核线程

kswap 用于在内存不足时进行内存回收, 每个NUMA内存节点对应一个 kswapd 内核线程

```bash
~$ ps -e | grep kswapd
   1400 ?        00:00:00 kswapd0
   1401 ?        00:00:00 kswapd1
   1402 ?        00:00:00 kswapd2
   1403 ?        00:00:00 kswapd3
```

> 如果系统中只有一个 node 的话就只会看到 kswap0

## kswap的初始化

系统初始化期间,调用kswapd_init,在每个内存节点上都会创建一个 kswap 内核线程, kswapd 也就是实际执行的主体函数

```c{18}
static int __init kswapd_init(void)
{
	int nid;

	swap_setup();
	for_each_node_state(nid, N_MEMORY)
 		kswapd_run(nid);
	return 0;
}
module_init(kswapd_init)

void __meminit kswapd_run(int nid)
{
	pg_data_t *pgdat = NODE_DATA(nid);

	pgdat_kswapd_lock(pgdat);
	if (!pgdat->kswapd) {
		pgdat->kswapd = kthread_run(kswapd, pgdat, "kswapd%d", nid);
		if (IS_ERR(pgdat->kswapd)) {
			/* failure at boot is fatal */
			BUG_ON(system_state < SYSTEM_RUNNING);
			pr_err("Failed to start kswapd on node %d\n", nid);
			pgdat->kswapd = NULL;
		}
	}
	pgdat_kswapd_unlock(pgdat);
}
```

> 每个内核线程的命名方式 `kswapd%d` 也对应了前文我们提到的可以在多 numa 中看到多个 kswapd 内核线程

## kswap的触发

kswap 虽然在系统启动时就会创建,但是大多数时候它处于睡眠状态,不会占用 CPU 资源。唤醒 kswapd 线程有主动和被动两种方式

- 被动唤醒: 进程申请内存失败, 唤醒 kswapd 回收内存
- 主动唤醒: 系统内存到达水位线(watermark), 主动唤醒 kswapd 回收内存

可以发现无论是主动还是被动, 唤醒 kswapd 线程的契机都是**可用内存资源不足**. 相比之下, 被动唤醒的过程 kswapd 线程回收内存的操作占用分配内存的关键路径, 因此主动唤醒并即使回收内存资源更为有利于系统运行稳定。

### 被动唤醒

简化后的页面分配的关键函数 __alloc_pages 分为两个阶段, 如果可以从 freelist 中直接找到空余空间则直接 fast path 返回 page, 否则需要进入 __alloc_pages_slowpath 找到可用页面

```c
struct page *__alloc_pages(gfp_t gfp, unsigned int order, int preferred_nid,
							nodemask_t *nodemask)
{
	struct page *page;
	// fast path
	page = get_page_from_freelist(alloc_gfp, order, alloc_flags, &ac);
	if (likely(page))
		goto out;
    // slow path
	page = __alloc_pages_slowpath(alloc_gfp, order, &ac);

out:
	return page;
}
```

> [!NOTE]
> order 参数指内存块包含 2^n 个连续的页
>
> 与 [numa](./numa.md) 和 [伙伴算法](./伙伴系统.md) 中我们介绍的 zone 里面的 freelist 结构体有关
>
> ```c
> struct zone {
>     //页面使用状态的信息,用于伙伴系统的,每个元素对应不同阶page大小
>     //目前系统上有11种不同大小的页面,从2^0 - 2^10,即4k-4M大小的页面
>     struct free_area	free_area[MAX_ORDER];
> };
> ```

slowpath 中和页面回收以及 kswapd 唤醒的相关函数调用如下

```txt
__alloc_pages_slowpath
    wake_all_kswapds----------------------------------- 路径1,进入慢分配路径会先唤醒kswap进程
        wakeup_kswapd
            wake_up_interruptible(&pgdat->kswapd_wait)
    __alloc_pages_direct_reclaim------------------------路径1 失败后,进行直接内存回收
        __perform_reclaim
            try_to_free_pages
                throttle_direct_reclaim
                    allow_direct_reclaim
                        wake_up_interruptible(&pgdat->kswapd_wait)
```

唤醒 kswapd 时会唤醒所有 nodemask 对应的 zone 的 node 上的 kswapd

> [!NOTE]
> 这里确实有点迷惑, 按理来说每个 node 上只有一个 kswapd, 该 node 上所有 zone 都对应同一个 node, 可以直接唤醒这个 node 的 kswapd 而不是遍历所有 zone, 这里不会出现多个 last_pgdat
>
> 唯一的可能是为了后续的 zone 扩展性?

```c{15,61}
static void wake_all_kswapds(unsigned int order, gfp_t gfp_mask,
			     const struct alloc_context *ac)
{
	struct zoneref *z;
	struct zone *zone;
	pg_data_t *last_pgdat = NULL;
	enum zone_type high_zoneidx = ac->high_zoneidx;
    //遍历每个zone,唤醒每个zone对应的kswap进程回收内存
    //有点奇怪,为啥不直接遍历node就好,还得从zone返回查找node
	for_each_zone_zonelist_nodemask(zone, z, ac->zonelist, highest_zoneidx,
					ac->nodemask) {
		if (!managed_zone(zone))
			continue;
		if (last_pgdat != zone->zone_pgdat) {
			wakeup_kswapd(zone, gfp_mask, order, highest_zoneidx);
			last_pgdat = zone->zone_pgdat;
		}
	}
}

void wakeup_kswapd(struct zone *zone, gfp_t gfp_flags, int order,
		   enum zone_type classzone_idx)
{
	pg_data_t *pgdat;

	if (!managed_zone(zone))
		return;

	if (!cpuset_zone_allowed(zone, gfp_flags))
		return;
    //获取该zone对应的node节点对象
	pgdat = zone->zone_pgdat;

	if (pgdat->kswapd_classzone_idx == MAX_NR_ZONES)
		pgdat->kswapd_classzone_idx = classzone_idx;
	else
		pgdat->kswapd_classzone_idx = max(pgdat->kswapd_classzone_idx,
						  classzone_idx);
    //kswap回收的内存不能小于进程分配的值,不然回收就没有意义了
	pgdat->kswapd_order = max(pgdat->kswapd_order, order);
    //如果该node的kswap进程已启动就返回
	if (!waitqueue_active(&pgdat->kswapd_wait))
		return;
		
    //如果此时kswap回收失败次数大于16次
    //或者有至少有一个zone在high_wmark条件下有满足此order的页面
    //则不必唤醒kswap进程,留给需要使用内存的进程自行处理
	if (pgdat->kswapd_failures >= MAX_RECLAIM_RETRIES ||
	    pgdat_balanced(pgdat, order, classzone_idx)) {
		
        //此时内存碎片太多,如果内存分配没有启动直接回收内存,
        //尝试启动内存整理进程,将碎片整理为连续内存,回收为高阶页面
		if (!(gfp_flags & __GFP_DIRECT_RECLAIM))
			wakeup_kcompactd(pgdat, order, classzone_idx);
		return;
	}

	trace_mm_vmscan_wakeup_kswapd(pgdat->node_id, classzone_idx, order,
				      gfp_flags);
    //唤醒在kswapd_wait队列上的kswap进程
	wake_up_interruptible(&pgdat->kswapd_wait);
}
```

当唤醒kswap返回后,会尝试分配内存,如果还是失败,会再一次切换zone,如果切换后还是无法分配出内存,就只能进行直接内存回收了(__alloc_pages_direct_reclaim)

```c{35,36}
static inline struct page *
__alloc_pages_slowpath(gfp_t gfp_mask, unsigned int order,
						struct alloc_context *ac)
{
    //如果允许唤醒kswap,先赶紧唤醒回收点内存
	if (gfp_mask & __GFP_KSWAPD_RECLAIM)
		wake_all_kswapds(order, gfp_mask, ac);

	//尝试是否能分配出来,就看kswap有没有那么快了,
	page = get_page_from_freelist(gfp_mask, order, alloc_flags, ac);
	if (page)
		goto got_pg;
    
retry:
    //再次确认kswap没有意外进入睡眠
	if (gfp_mask & __GFP_KSWAPD_RECLAIM)
		wake_all_kswapds(order, gfp_mask, ac);

	reserve_flags = __gfp_pfmemalloc_flags(gfp_mask);
	if (reserve_flags)
		alloc_flags = reserve_flags;

	//尝试换个zone
	if (!(alloc_flags & ALLOC_CPUSET) || reserve_flags) {
		ac->preferred_zoneref = first_zones_zonelist(ac->zonelist,
					ac->high_zoneidx, ac->nodemask);
	}

	//在新的zone上分配内存
	page = get_page_from_freelist(gfp_mask, order, alloc_flags, ac);
	if (page)
		goto got_pg;
    
    //到现在还是没法满足用户需求,直接内存回收吧
	page = __alloc_pages_direct_reclaim(gfp_mask, order, alloc_flags, ac,
							&did_some_progress);
	if (page)
		goto got_pg;

    //内存碎片整理
	page = __alloc_pages_direct_compact(gfp_mask, order, alloc_flags, ac,
					compact_priority, &compact_result);
	if (page)
		goto got_pg;
}
```

直接回收内存阻塞在于 throttle_direct_reclaim,它会一直唤醒kswap回收内存,直到空闲内存满足要求才返回

```txt
__alloc_pages_direct_reclaim
    __perform_reclaim
        try_to_free_pages
            throttle_direct_reclaim
```

```c{18-25}
static bool throttle_direct_reclaim(gfp_t gfp_mask, struct zonelist *zonelist,
					nodemask_t *nodemask)
{
	//唤醒当前zonelist上对应的kswap进程
	for_each_zone_zonelist_nodemask(zone, z, zonelist,
					gfp_zone(gfp_mask), nodemask) {
		if (zone_idx(zone) > ZONE_NORMAL)
			continue;

		//唤醒第一个可用的node对应的kswap进程,如果内存足够了,直接退出
        //就不需要进程后面的阻塞性内存回收
		pgdat = zone->zone_pgdat;
		if (allow_direct_reclaim(pgdat))
			goto out;
		break;
	}
	//如果当前内存分配调用者无法进入文件系统,阻塞1s,让kswap回收内存,然后唤醒进程
	if (!(gfp_mask & __GFP_FS))
		wait_event_interruptible_timeout(pgdat->pfmemalloc_wait,
			allow_direct_reclaim(pgdat), HZ);
	else
		//如果内存分配调用者可以进入文件系统,那在这里阻塞等待,直到kswap回收到足够内存,
        //再唤醒进程,否则就通过allow_direct_reclaim不停的唤醒kswap回收内存
		wait_event_killable(zone->zone_pgdat->pfmemalloc_wait,
			allow_direct_reclaim(pgdat));
}
```

## kswapd 工作流程

kswapd 的工作流程如下:

1. 监测内存状态: 系统中的空闲内存分为几个水位(high、low、min).当可用内存低于 low 水位时,kswapd 被唤醒。
2. 启动回收操作:kswapd 调用内核中的页面回收函数,如 shrink_node() 和 shrink_zone(),尝试回收页面以恢复到 high 水位。
3. 页面分类:页面分为 匿名页面(如进程堆栈)和 文件缓存页面(如文件数据).kswapd 优先回收文件缓存页面,因为这些页面可以快速从磁盘重新加载。
4. 直接回收(Direct Reclaim):如果 kswapd 无法及时释放足够的页面,并且分配内存的进程也遇到了内存不足的情况,进程会触发直接回收,这是更高成本的操作

```bash
kswapd_init---------------------------------kswapd进程初始化
  kswapd_run--------------------------------为每个内存节点创建kswapd内核进程
    kswapd----------------------------------kswapd内核进程主体函数
      kswapd_try_to_sleep-------------------kswapd尝试睡眠并且让出CPU
      balance_pgdat-------------------------kswapd无法睡眠,开始对当前节点回收页面
        kswapd_shrink_node------------------针对该节点回收内存页面
          pgdat_balanced--------------------检查当前内存节点是否处于高水位
          shrink_node-----------------------不满足高水位要求,开始扫描回收的页面
            shrink_node_memcg---------------遍历扫描该节点各个内存cgroup,回收页面
              shrink_list-------------------扫描各种LRU链表,尝试回收页面
                shrink_active_list----------扫描active LRU链表,把一些active页面迁移到inactive链表中
                shrink_inactive_list--------扫描inactive LRU链表,尝试回收页面
                  shrink_page_list----------扫描并尝试回收从inactive链表隔离出来的page_list链表的页面
            shrink_slab---------------------调用系统注册的所有shrinker,回收slab缓存

```

在 [numa](./numa.md) 中提到每一个 numa node 对应一个 `pglist_data` 结构体对象, 其中和 kswap 相关的字段如下

```c
typedef struct pglist_data {
	wait_queue_head_t kswapd_wait;          //kswapd进程的等待队列
	wait_queue_head_t pfmemalloc_wait;      //直接内存回收过程中的进程等待队列
	struct task_struct *kswapd;	            //指向该结点的kswapd进程的task_struct
	int kswapd_order;                       //kswap回收页面大小
    enum zone_type kswapd_classzone_idx;    //kswap扫描的内存域范围
    int kswapd_failures;                    //kswap失败次数
} pg_data_t;
```


页面回收算法尽量不scan整个系统的全部进程地址空间,毕竟那是一个比较笨的办法。回收算法可以考虑收缩内存cache,也可以遍历inactive_list来试图完成本次reclaim数目的要求(该链表中有些page不和任何进程相关),如果通过这些方法释放了足够多的page frame,那么一切都搞定了,不需要scan进程地址空间。当然,情况并非总是那么美好,有时候,必须启动进程物理页面回收过程才能满足页面回收的要求。

## 参考

- [kswapd进程工作原理(一)_初始化及触发](https://blog.csdn.net/u010039418/article/details/103443581)
- [kswapd进程工作原理(四)_总结](https://blog.csdn.net/u010039418/article/details/114456268)