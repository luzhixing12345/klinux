
# migration

页面迁移允许在进程运行时在 NUMA 系统中的节点之间移动页面的物理位置.这意味着进程看到的虚拟地址不会改变.但是,系统会重新排列这些页面的物理位置.页面迁移的主要目的是通过将页面移动到访问内存的进程正在运行的处理器附近来减少内存访问的延迟.后来内存规整和内存热插拔等场景都使用了此功能.

内存迁移相关的有两个系统调用函数 migrate_pages 和 move_pages, migrate_pages 表示将一个进程的所有内存迁移到另一组节点. 而 move_pages 更细粒度一些, 表示将一个进程的一些页面迁移到其他节点

```c
#include <numaif.h>

long migrate_pages(int pid, unsigned long maxnode,
                   const unsigned long *old_nodes,
                   const unsigned long *new_nodes);

long move_pages(int pid, unsigned long count, void **pages,
                       const int *nodes, int *status, int flags);
// Link with -lnuma.
```

两个系统调用在功能上有相似之处, 事实上他们在内核中都会调用内核函数 migrate_pages 完成页面的迁移, 下面我们来详细介绍一下这个函数. migrate_pages 经过一次参数的变化, 添加了 ret_succeeded 表示成功迁移的页面个数, 也就是说并不是所有页面都一定迁移成功, 有可能因为一些原因迁移失败

```c
// v5.x
int migrate_pages(struct list_head *from, new_page_t get_new_page,
		free_page_t put_new_page, unsigned long private,
		enum migrate_mode mode, int reason);
// v5.18
int migrate_pages(struct list_head *from, new_folio_t get_new_folio,
		free_folio_t put_new_folio, unsigned long private,
		enum migrate_mode mode, int reason, unsigned int *ret_succeeded);
```

- from: 迁移页面的链表.
- get_new_page:申请新内存的页面的函数指针
- put_new_page: 迁移失败时释放目标页面的函数指针
- private: 传递给get_new_page的参数.
- mode: 迁移模式
- reason: 迁移因素

其中 mode 和 reason 有很多种

| **迁移类型**           | **描述**                                                                                      |
|------------------------|-----------------------------------------------------------------------------------------------|
| **MIGRATE_ASYNC**          | 异步迁移,过程中不会发生阻塞.                                                               |
| MIGRATE_SYNC_LIGHT     | 轻度同步迁移,允许大部分的阻塞操作,但不允许脏页的回写操作.                                  |
| **MIGRATE_SYNC**           | 同步迁移,迁移过程会发生阻塞.如果某个页正在 writeback 或被锁定,会等待完成后再迁移.        |
| MIGRATE_SYNC_NO_COPY   | 同步迁移,但不等待页面的拷贝过程.页面拷贝通过回调 `migratepage()`,过程可能涉及 DMA.      |

| **迁移原因**           | **描述**                                                                                      |
|------------------------|-----------------------------------------------------------------------------------------------|
| MR_COMPACTION          | 内存规整导致的迁移.                                                                          |
| MR_MEMORY_FAILURE      | 内存出现硬件问题(如 ECC 校验失败)时触发的页面迁移(`memory–failure.c`).                   |
| MR_MEMORY_HOTPLUG      | 内存热插拔导致的迁移.                                                                        |
| MR_SYSCALL             | 应用层主动调用 `migrate_pages()` 或 `move_pages()` 触发的迁移.                               |
| MR_MEMPOLICY_MBIND     | 调用 `mbind` 系统调用设置 memory policy 时触发的迁移.                                        |
| **MR_NUMA_MISPLACED**      | NUMA balance 触发的页面迁移.                                                                 |
| MR_CONTIG_RANGE        | 调用 `alloc_contig_range()` 为 CMA 或 HugeTLB 分配连续内存时触发的迁移(与内存规整相关).    |

> 系统中绝大部分导致迁移的情况是因为 numa balance 产生的 MR_NUMA_MISPLACED 页面迁移, 在后文会详细讨论

## 函数执行流程

在正式介绍 migrate_pages 的函数执行流程之前我们先来做一个小实验测试一下 migrate_pages 内部各个函数的执行时间占比, 对应的代码如下

> 这段代码内容比较简单, 创建 NUM_PAGES 个页面, 使用系统调用将它们迁移到其他 node 节点上, 循环 count 次

```c
#define _GNU_SOURCE
#include <errno.h>
#include <numa.h>
#include <numaif.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

int main() {
    const int NUM_PAGES = 5000;
    const int PAGE_SIZE = 4096;
    void *pages[NUM_PAGES];
    int status[NUM_PAGES];
    int nodes[NUM_PAGES];
    int pid = getpid();  // Current process ID

    // Allocate memory for pages
    int node_number = numa_max_node() + 1;
    printf("node_number: %d\n", node_number);

    int count = 1000;
    while (count--) {
        // printf("count: %d\n", count);
        for (int i = 0; i < NUM_PAGES; i++) {
            pages[i] = malloc(PAGE_SIZE);
            if (!pages[i]) {
                perror("malloc");
                return EXIT_FAILURE;
            }
            // Initialize memory
            *((char *)pages[i]) = (char)i;
            nodes[i] = i % node_number;  // Move pages to NUMA node
        }

        // Use move_pages system call to move pages
        if (syscall(SYS_move_pages, pid, NUM_PAGES, pages, nodes, status, 0) == -1) {
            perror("move_pages");
            return EXIT_FAILURE;
        }

        // Check the status of each page
        for (int i = 0; i < NUM_PAGES; i++) {
            if (status[i] == -1) {
                printf("Page %d: Failed to move (errno: %d)\n", i, errno);
            } else {
                // printf("Page %d: Successfully moved to node %d\n", i, status[i]);
            }
        }

        // Free allocated memory
        for (int i = 0; i < NUM_PAGES; i++) {
            free(pages[i]);
        }
    }
    return EXIT_SUCCESS;
}
```

性能分析的结果如下所示, 其中可以看到占用时间最长的六个函数(不代表执行顺序, 仅为占比大小顺序排列)

![20241113100106](https://raw.githubusercontent.com/learner-lu/picbed/master/20241113100106.png)

migrate_pages 分为如下几个步骤, 分别对应

1. 分配 new_pages: `alloc_migration_target`
2. 获取 old page 的页面锁 PG_locked: `folio_trylock(src)`
3. 若是正在writeback的页面,则根据迁移模式判断是否等待页面wirteback(MIGRATE_SYNC_LIGHT和MIGRATE_ASYNC不等待) `folio_test_writeback(src)`
4. 获取 new page 的页面锁 PG_locked: `folio_trylock(dst)`
5. 取消页面映射: `try_to_migrate`

move_pages 最终会调用到 do_move_pages_to_node, 进一步调用 migrate_pages, 参数使用了 MIGRATE_SYNC 和 MR_SYSCALL 表示通过系统调用进行同步迁移.

migrate_pages 的形参 get_new_page 对应 alloc_migration_target 函数, 其作用是在目标节点上分配新页面

```c{18,19}
struct folio *alloc_migration_target(struct folio *src, unsigned long private)
{
	struct migration_target_control *mtc;
	mtc = (struct migration_target_control *)private;
	gfp_mask = mtc->gfp_mask;
	return __folio_alloc(gfp_mask, order, nid, mtc->nmask);
}

static int do_move_pages_to_node(struct mm_struct *mm,
		struct list_head *pagelist, int node)
{
	int err;
	struct migration_target_control mtc = {
		.nid = node,
		.gfp_mask = GFP_HIGHUSER_MOVABLE | __GFP_THISNODE,
	};

	err = migrate_pages(pagelist, alloc_migration_target, NULL,
		(unsigned long)&mtc, MIGRATE_SYNC, MR_SYSCALL, NULL);
	if (err)
		putback_movable_pages(pagelist);
	return err;
}
```

---

migrate_pages 中首先计算一共需要迁移的页面数量(nr_pages), 如果小于 `NR_MAX_BATCHED_MIGRATION`(512) 在将其从 from 转移到 folios 中; 根据 mode 的类型, 如果是异步(ASYNC)则使用 migrate_pages_batch, 否则使用 migrate_pages_sync 进行同步迁移. 如果迁移后 from 仍不为空则重新尝试迁移

```c{24-32}
int migrate_pages(struct list_head *from, new_folio_t get_new_folio,
		free_folio_t put_new_folio, unsigned long private,
		enum migrate_mode mode, int reason, unsigned int *ret_succeeded)
{
    struct folio *folio, *folio2;
	LIST_HEAD(folios);
again:
	nr_pages = 0;
	list_for_each_entry_safe(folio, folio2, from, lru) {
		/* Retried hugetlb folios will be kept in list  */
		if (folio_test_hugetlb(folio)) {
			list_move_tail(&folio->lru, &ret_folios);
			continue;
		}

		nr_pages += folio_nr_pages(folio);
		if (nr_pages >= NR_MAX_BATCHED_MIGRATION)
			break;
	}
	if (nr_pages >= NR_MAX_BATCHED_MIGRATION)
		list_cut_before(&folios, from, &folio2->lru);
	else
		list_splice_init(from, &folios);
	if (mode == MIGRATE_ASYNC)
		rc = migrate_pages_batch(&folios, get_new_folio, put_new_folio,
				private, mode, reason, &ret_folios,
				&split_folios, &stats,
				NR_MAX_MIGRATE_PAGES_RETRY);
	else
		rc = migrate_pages_sync(&folios, get_new_folio, put_new_folio,
				private, mode, reason, &ret_folios,
				&split_folios, &stats);

	rc_gather += rc;
	if (!list_empty(from))
		goto again;
}
```

> [!NOTE]
> 在 `migrate_pages` 函数中,`split_folios` 和 `ret_folios` 是两个链表(list_head)类型的变量,它们用于管理folio(页框)的迁移过程.
> 
> - `split_folios` 用于存储那些需要被拆分(split)的folio.这些folio通常是大页(large page)或者巨页(huge page),它们需要被拆分成小页(small page)才能迁移到新的位置.
> - `ret_folios` 用于存储那些已经迁移到新位置的folio.这些folio可能是原来的大页、巨页或者小页,它们已经被成功迁移到新的位置.
>
> 当 `migrate_pages` 函数成功迁移一个folio时,它会将该folio添加到 `ret_folios` 链表中.这个链表用于存储所有已经迁移的folio,以便函数可以返回给调用者.

### 同步迁移

因为同步迁移的代码比较简单所以先来看同步迁移. migrate_pages_sync 用于同步地将一组folios(页帧)从一个位置迁移到另一个位置 

1. 首先,它尝试使用`MIGRATE_ASYNC`模式批量迁移folios
2. 如果批量迁移失败,则退回到逐个同步迁移每个folio

```c{32-42,13-15}
static int migrate_pages_sync(struct list_head *from, new_folio_t get_new_folio,
		free_folio_t put_new_folio, unsigned long private,
		enum migrate_mode mode, int reason,
		struct list_head *ret_folios, struct list_head *split_folios,
		struct migrate_pages_stats *stats)
{
	int rc, nr_failed = 0;
	LIST_HEAD(folios);
	struct migrate_pages_stats astats;

	memset(&astats, 0, sizeof(astats));
	/* Try to migrate in batch with MIGRATE_ASYNC mode firstly */
	rc = migrate_pages_batch(from, get_new_folio, put_new_folio, private, MIGRATE_ASYNC,
				 reason, &folios, split_folios, &astats,
				 NR_MAX_MIGRATE_ASYNC_RETRY);
	stats->nr_succeeded += astats.nr_succeeded;
	stats->nr_thp_succeeded += astats.nr_thp_succeeded;
	stats->nr_thp_split += astats.nr_thp_split;
	if (rc < 0) {
		stats->nr_failed_pages += astats.nr_failed_pages;
		stats->nr_thp_failed += astats.nr_thp_failed;
		list_splice_tail(&folios, ret_folios);
		return rc;
	}
	stats->nr_thp_failed += astats.nr_thp_split;
	nr_failed += astats.nr_thp_split;
	/*
	 * Fall back to migrate all failed folios one by one synchronously. All
	 * failed folios except split THPs will be retried, so their failure
	 * isn't counted
	 */
	list_splice_tail_init(&folios, from);
	while (!list_empty(from)) {
		list_move(from->next, &folios);
		rc = migrate_pages_batch(&folios, get_new_folio, put_new_folio,
					 private, mode, reason, ret_folios,
					 split_folios, stats, NR_MAX_MIGRATE_SYNC_RETRY);
		list_splice_tail_init(&folios, ret_folios);
		if (rc < 0)
			return rc;
		nr_failed += rc;
	}

	return nr_failed;
}
```

我们详细的看一下同步迁移的部分, 这里涉及到了几个链表(list_head 相关的宏)

- `list_splice_tail_init(&folios, from)` 将 `folios` 链表中的所有元素追加到 `from` 链表的尾部,并清空 `folios` 链表.
- 此时 `from` 链表中保存着所有需要迁移的页面, `folios` 为空
- 如果 `from` 链表不空
  * `list_move(from->next, &folios)` 将 `from` 链表中的下一个元素移动到 `folios` 链表中.
  * `rc = migrate_pages_batch(&folios, ...)` 尝试批量迁移 `folios` 链表中的元素.
  * `list_splice_tail_init(&folios, ret_folios)` 将 `folios` 链表中的所有元素追加到 `ret_folios` 链表的尾部,并清空 `folios` 链表.

其中每一步的 list_move 只会将 from 的第一个元素移动到 folios 中, 所以**每次循环也只会移动一个 page, 因此称为同步**

### 异步迁移

在同步迁移中我们注意到它其实也是先尝试进行异步迁移, 这说明**异步迁移的性能是要好于同步迁移的**. 这部分代码比较多我们分几部分介绍.

首先异步迁移可能会失败, 如果失败将重试最多 `NR_MAX_MIGRATE_PAGES_RETRY`(10) 次. 在开头判断如果迁移类型不是 `ASYNC` 并且 from 链表不空且不唯一(即有很多个页面, 但不是异步) 则会报错

```c
#define NR_MAX_MIGRATE_PAGES_RETRY 10

static int migrate_pages_batch(struct list_head *from,
		new_folio_t get_new_folio, free_folio_t put_new_folio,
		unsigned long private, enum migrate_mode mode, int reason,
		struct list_head *ret_folios, struct list_head *split_folios,
		struct migrate_pages_stats *stats, int nr_pass)
{

	VM_WARN_ON_ONCE(mode != MIGRATE_ASYNC &&
			!list_empty(from) && !list_is_singular(from));

	for (pass = 0; pass < nr_pass && retry; pass++) {
        // ...
    }
	return rc;
}
```

- migrate_pages_batch
  - migrate_folio_unmap
    - dst = get_new_folio(src, private) 分配新页面 `alloc_migration_target`
    - folio_trylock(src) 获取 src 锁
    - folio_test_writeback(src) 等待脏页面写回
    - folio_trylock(dst) 获取 dst 锁
    - try_to_migrate 
  - migrate_folio_move
    - move_to_new_folio 拷贝old page的内容和struct page元数据到new page
    - folio_add_lru 新页面加到 lru 中
    - remove_migration_ptes 迁移页表,通过反向映射机制RMAP来建立new page的映射关系
    - migrate_folio_done 清空 src folio

## 页面降级(demotion)

系统接口中的 demotion_enabled 变量可以控制是否降级页面, 可以将其设置为 true/false 来选择开启/关闭页面降级功能

```bash
$ ls /sys/kernel/mm/numa/
demotion_enabled
$ cat /sys/kernel/mm/numa/demotion_enabled
false
```

其对应内核中的 numa_demotion_enabled 变量, 改变量的唯一影响在 mm/vmscan.c 的 `can_demotion` 函数

```c
static bool can_demote(int nid, struct scan_control *sc)
{
	if (!numa_demotion_enabled)
		return false;
	if (sc && sc->no_demotion)
		return false;
	if (next_demotion_node(nid) == NUMA_NO_NODE)
		return false;

	return true;
}
```

## tracing

```c
int migrate_pages(struct list_head *from, new_folio_t get_new_folio,
		free_folio_t put_new_folio, unsigned long private,
		enum migrate_mode mode, int reason, unsigned int *ret_succeeded)
{

	trace_mm_migrate_pages_start(mode, reason);
    // ...
	trace_mm_migrate_pages(stats.nr_succeeded, stats.nr_failed_pages,
			       stats.nr_thp_succeeded, stats.nr_thp_failed,
			       stats.nr_thp_split, mode, reason);
}
```

```bash
$ sudo cat /sys/kernel/tracing/available_events | grep migrate
sched:sched_migrate_task
compaction:mm_compaction_migratepages
compaction:mm_compaction_isolate_migratepages
migrate:remove_migration_pte
migrate:set_migration_pte
migrate:mm_migrate_pages_start
migrate:mm_migrate_pages
syscalls:sys_exit_migrate_pages
syscalls:sys_enter_migrate_pages
```

```bash
echo 1 | sudo tee /sys/kernel/tracing/events/mm/migrate_pages_start/enable
echo 1 | sudo tee /sys/kernel/tracing/tracing_on
sudo cat /sys/kernel/tracing/trace
```

```bash
# 停止跟踪
echo 0 | sudo tee /sys/kernel/tracing/tracing_on

# 查看或保存跟踪数据
sudo cat /sys/kernel/tracing/trace > /path/to/save/trace_output.txt
```

## 参考

- [Linux内存管理(十): 页面迁移](https://blog.csdn.net/yhb1047818384/article/details/119920971)
- [linux那些事之页迁移(page migratiom)](https://www.cnblogs.com/linhaostudy/p/17647370.html)
- [page_migration](https://docs.kernel.org/mm/page_migration.html)
- [Heterogeneous Memory Management (HMM)](https://docs.kernel.org/mm/hmm.html)