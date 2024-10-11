
# kernel-code

- pid -> task
- task: get_task_struct
- task -> mm
- mm + addr -> vma

```c
#include <linux/sche.h>

/* Find the mm_struct */
rcu_read_lock();
task = pid ? find_task_by_vpid(pid) : current;
if (!task) {
    rcu_read_unlock();
    err = -ESRCH;
    goto out;
}
get_task_struct(task);
mm = get_task_mm(task);
```

```c
struct mm_struct *mm;
int err;
nodemask_t task_nodes;
mm = find_mm_struct(pid, &task_nodes);
```

```c
vma = find_vma(mm, addr);
```

- vma_lookup() - Find a VMA at a specific address
- find_vma() - Look up the first VMA which satisfies  addr < vm_end,  NULL if none

vma + addr -> page

```bash
page = follow_page(vma, addr, FOLL_GET | FOLL_DUMP);
```

```c
struct mm_walk_ops *ops;
err = walk_page_range(mm, start, end, ops, &qp);
```

从用户空间拿数据, do_pages_move()

```c
/**
 * get_user - Get a simple variable from user space.
 * @x:   Variable to store result.
 * @ptr: Source address, in user space.
 *
 * Context: User context only. This function may sleep if pagefaults are
 *          enabled.
 *
 * This macro copies a single simple variable from user space to kernel
 * space.  It supports simple types like char and int, but not larger
 * data types like structures or arrays.
 *
 * @ptr must have pointer-to-simple-variable type, and the result of
 * dereferencing @ptr must be assignable to @x without a cast.
 *
 * Return: zero on success, or -EFAULT on error.
 * On error, the variable @x is set to zero.
 */
#define get_user(x,ptr) ({ might_fault(); do_get_user_call(get_user,x,ptr); })
```