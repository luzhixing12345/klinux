/*
 *  linux/mm/memory.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * demand-loading started 01.12.91 - seems it is high on the list of
 * things wanted, and it should be easy to implement. - Linus
 */

/*
 * Ok, demand-loading was easy, shared pages a little bit tricker. Shared
 * pages started 02.12.91, seems to work. - Linus.
 *
 * Tested sharing by executing about 30 /bin/sh: under the old kernel it
 * would have taken more than the 6M I have free, but it worked well as
 * far as I could see.
 *
 * Also corrected some "invalidate()"s - I wasn't doing enough of them.
 */

#include <asm/system.h>
#include <linux/head.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <signal.h>

void do_exit(long code);  // kernel/exit.c

static inline void oom(void) {
    printk("out of memory\n\r");
    do_exit(SIGSEGV);  // segment violation 无效内存引用
}

// 刷新页变换高速缓冲宏函数.
// 为了提高地址转换的效率,CPU 将最近使用的页表数据存放在芯片中高速缓冲中.在修改过页表
// 信息之后,就需要刷新该缓冲区.这里使用重新加载页目录基址寄存器 cr3 的方法来进行刷新.
// 下面 eax = 0,是页目录的基址.

#define invalidate() __asm__("movl %%eax,%%cr3" ::"a"(0))

// 下面的定义需要和 head.s 中的定义配合
#define LOW_MEM       0x100000                  // 内存低端(1MB)
#define PAGING_MEMORY (15 * 1024 * 1024)        // 主内存区最多 15MB
#define PAGING_PAGES  (PAGING_MEMORY >> 12)     // 分页后的物理内存页数
#define MAP_NR(addr)  (((addr)-LOW_MEM) >> 12)  // 指定内存地址映射为页号
#define USED          100                       // 页面被占用的标志

// 判断给定地址是否位于当前进程的代码段中
#define CODE_SPACE(addr) ((((addr) + 4095) & ~4095) < current->start_code + current->end_code)

// 全局变量，存放实际物理内存最高端地址
static long HIGH_MEMORY = 0;

// 复制 1 页内存（4K 字节）
#define copy_page(from, to) __asm__("cld ; rep ; movsl" ::"S"(from), "D"(to), "c"(1024))

// 内存映射字节图(1 字节代表 1 页内存)，每个页面对应的字节用于标志页面当前被引用（占用）次数。
static unsigned char mem_map[PAGING_PAGES] = {
    0,
};

/*
 * Get physical address of first (actually last :-) free page, and mark it
 * used. If no free pages left, return 0.
 */
unsigned long get_free_page(void) {
    register unsigned long __res asm("ax");

    __asm__(
        "std ; repne ; scasb\n\t"
        "jne 1f\n\t"
        "movb $1,1(%%edi)\n\t"
        "sall $12,%%ecx\n\t"
        "addl %2,%%ecx\n\t"
        "movl %%ecx,%%edx\n\t"
        "movl $1024,%%ecx\n\t"
        "leal 4092(%%edx),%%edi\n\t"
        "rep ; stosl\n\t"
        " movl %%edx,%%eax\n"
        "1: cld"
        : "=a"(__res)
        : "0"(0), "i"(LOW_MEM), "c"(PAGING_PAGES), "D"(mem_map + PAGING_PAGES - 1));
    return __res;
}

/*
 * Free a page of memory at physical address 'addr'. Used by
 * 'free_page_tables()'
 */
void free_page(unsigned long addr) {
    // 如果物理地址 addr 小于内存低端（1MB），则返回
    if (addr < LOW_MEM) {
        return;
    }
    // 如果物理地址 addr>=内存最高端，则显示出错信息
    if (addr >= HIGH_MEMORY) {
        panic("trying to free nonexistent page");
    }

    // 物理地址减去低端内存位置，再除以 4KB，得页面号
    addr = (addr - LOW_MEM) >> 12;
    // 如果对应内存页面映射字节不等于 0，则减 1 返回
    if (mem_map[addr]) {
        mem_map[addr]--;
        return;
    } else {
        // 否则置对应页面映射字节为 0，并显示出错信息，死机
        mem_map[addr] = 0;
        panic("trying to free free page");
    }
}

/**
 * @brief 根据指定的线性地址和限长（页表个数），释放对应内存页表所指定的内存块并置表项空闲
 *
 * @param from 起始基地址
 * @param size 释放的长度
 * @return int
 */
int free_page_tables(unsigned long from, unsigned long size) {
    unsigned long *pg_table;
    unsigned long *dir, nr;

    // 要释放内存块的地址需以 4M 为边界
    if (from & 0x3fffff) {
        panic("free_page_tables called with wrong alignment");
    }
    // 出错，试图释放内核和缓冲所占空间
    if (!from) {
        panic("Trying to free up swapper memory space");
    }
    // 计算所占页目录项数(4M 的进位整数倍)，也即所占页表数
    size = (size + 0x3fffff) >> 22;

    // 对应的目录项号=from>>22, 每项占 4 字节, 故 from >> 22 << 2 => from >> 20
    // &0xffc 确保目录项指针范围有效
    // 页目录从物理地址 0 开始的
    dir = (unsigned long *)((from >> 20) & 0xffc); /* _pg_dir = 0 */
    for (; size-- > 0; dir++) {
        // 如果该目录项无效(目录项的位0(P位)表示对应页表是否存在)，则继续
        if (!(1 & *dir)) {
            continue;
        }
        // 取得目录项中的页表地址(31-12)
        pg_table = (unsigned long *)(0xfffff000 & *dir);
        for (nr = 0; nr < 1024; nr++) {
            // 如果该页表项有效(P 位=1)，则释放对应内存页
            if (1 & *pg_table) {
                free_page(0xfffff000 & *pg_table);
            }
            *pg_table = 0;  // 页表项内容清零
            pg_table++;     // 指向下一项
        }
        free_page(0xfffff000 & *dir);  // 释放该页表所占内存页面
        *dir = 0;                      // 对相应页表的目录项清零
    }
    invalidate();  //  刷新页变换高速缓冲
    return 0;
}

/*
 *  Well, here is one of the most complicated functions in mm. It
 * copies a range of linerar addresses by copying only the pages.
 * Let's hope this is bug-free, 'cause this one I don't want to debug :-)
 *
 * Note! We don't copy just any chunks of memory - addresses have to
 * be divisible by 4Mb (one page-directory entry), as this makes the
 * function easier. It's used only by fork anyway.
 *
 * NOTE 2!! When from==0 we are copying kernel space for the first
 * fork(). Then we DONT want to copy a full page-directory entry, as
 * that would lead to some serious memory waste - we just copy the
 * first 160 pages - 640kB. Even that is more than we need, but it
 * doesn't take any more memory - we don't copy-on-write in the low
 * 1 Mb-range, so the pages can be shared with the kernel. Thus the
 * special case for nr=xxxx.
 */
int copy_page_tables(unsigned long from, unsigned long to, long size) {
    unsigned long *from_page_table;
    unsigned long *to_page_table;
    unsigned long this_page;
    unsigned long *from_dir, *to_dir;
    unsigned long nr;

    // 源地址和目的地址都需要是在 4Mb 的内存边界地址上。否则出错，死机
    if ((from & 0x3fffff) || (to & 0x3fffff)) {
        panic("copy_page_tables called with wrong alignment");
    }
    // 取得源地址和目的地址的目录项指针(from_dir 和 to_dir)
    from_dir = (unsigned long *)((from >> 20) & 0xffc);
    to_dir = (unsigned long *)((to >> 20) & 0xffc);
    size = ((unsigned)(size + 0x3fffff)) >> 22;
    for (; size-- > 0; from_dir++, to_dir++) {
        //  如果目的目录项指定的页表已经存在(P=1)，则出错，死机。
        if (1 & *to_dir) {
            panic("copy_page_tables: already exist");
        }
        // 如果此源目录项未被使用，则不用复制对应页表，跳过
        if (!(1 & *from_dir)) {
            continue;
        }
        // 取当前源目录项中页表的地址
        from_page_table = (unsigned long *)(0xfffff000 & *from_dir);
        // 为目的页表取一页空闲内存，如果返回是 0 则说明没有申请到空闲内存页面。返回值=-1，退出
        if (!(to_page_table = (unsigned long *)get_free_page())) {
            return -1; /* Out of memory, see freeing */
        }
        // 设置目的目录项信息。7 是标志信息，表示(Usr, R/W, Present)
        *to_dir = ((unsigned long)to_page_table) | 7;
        nr = (from == 0) ? 0xA0 : 1024;
        for (; nr-- > 0; from_page_table++, to_page_table++) {
            this_page = *from_page_table;
            if (!(1 & this_page))
                continue;
            this_page &= ~2;
            *to_page_table = this_page;
            if (this_page > LOW_MEM) {
                *from_page_table = this_page;
                this_page -= LOW_MEM;
                this_page >>= 12;
                mem_map[this_page]++;
            }
        }
    }
    invalidate();
    return 0;
}

/*
 * This function puts a page in memory at the wanted address.
 * It returns the physical address of the page gotten, 0 if
 * out of memory (either when trying to access page-table or
 * page.)
 */
unsigned long put_page(unsigned long page, unsigned long address) {
    unsigned long tmp, *page_table;

    /* NOTE !!! This uses the fact that _pg_dir=0 */

    if (page < LOW_MEM || page >= HIGH_MEMORY)
        printk("Trying to put page %p at %p\n", page, address);
    if (mem_map[(page - LOW_MEM) >> 12] != 1)
        printk("mem_map disagrees with %p at %p\n", page, address);
    page_table = (unsigned long *)((address >> 20) & 0xffc);
    if ((*page_table) & 1)
        page_table = (unsigned long *)(0xfffff000 & *page_table);
    else {
        if (!(tmp = get_free_page()))
            return 0;
        *page_table = tmp | 7;
        page_table = (unsigned long *)tmp;
    }
    page_table[(address >> 12) & 0x3ff] = page | 7;
    /* no need for invalidate */
    return page;
}

void un_wp_page(unsigned long *table_entry) {
    unsigned long old_page, new_page;

    old_page = 0xfffff000 & *table_entry;
    if (old_page >= LOW_MEM && mem_map[MAP_NR(old_page)] == 1) {
        *table_entry |= 2;
        invalidate();
        return;
    }
    if (!(new_page = get_free_page()))
        oom();
    if (old_page >= LOW_MEM)
        mem_map[MAP_NR(old_page)]--;
    *table_entry = new_page | 7;
    invalidate();
    copy_page(old_page, new_page);
}

/*
 * This routine handles present pages, when users try to write
 * to a shared page. It is done by copying the page to a new address
 * and decrementing the shared-page counter for the old page.
 *
 * If it's in code space we exit with a segment error.
 */
void do_wp_page(unsigned long error_code, unsigned long address) {
#if 0
/* we cannot do this yet: the estdio library writes to code space */
/* stupid, stupid. I really want the libc.a from GNU */
	if (CODE_SPACE(address))
		do_exit(SIGSEGV);
#endif
    un_wp_page(
        (unsigned long *)(((address >> 10) & 0xffc) + (0xfffff000 & *((unsigned long *)((address >> 20) & 0xffc)))));
}

void write_verify(unsigned long address) {
    unsigned long page;

    if (!((page = *((unsigned long *)((address >> 20) & 0xffc))) & 1))
        return;
    page &= 0xfffff000;
    page += ((address >> 10) & 0xffc);
    if ((3 & *(unsigned long *)page) == 1) /* non-writeable, present */
        un_wp_page((unsigned long *)page);
    return;
}

void get_empty_page(unsigned long address) {
    unsigned long tmp;

    if (!(tmp = get_free_page()) || !put_page(tmp, address)) {
        free_page(tmp); /* 0 is ok - ignored */
        oom();
    }
}

/*
 * try_to_share() checks the page at address "address" in the task "p",
 * to see if it exists, and if it is clean. If so, share it with the current
 * task.
 *
 * NOTE! This assumes we have checked that p != current, and that they
 * share the same executable.
 */
static int try_to_share(unsigned long address, struct task_struct *p) {
    unsigned long from;
    unsigned long to;
    unsigned long from_page;
    unsigned long to_page;
    unsigned long phys_addr;

    from_page = to_page = ((address >> 20) & 0xffc);
    from_page += ((p->start_code >> 20) & 0xffc);
    to_page += ((current->start_code >> 20) & 0xffc);
    /* is there a page-directory at from? */
    from = *(unsigned long *)from_page;
    if (!(from & 1))
        return 0;
    from &= 0xfffff000;
    from_page = from + ((address >> 10) & 0xffc);
    phys_addr = *(unsigned long *)from_page;
    /* is the page clean and present? */
    if ((phys_addr & 0x41) != 0x01)
        return 0;
    phys_addr &= 0xfffff000;
    if (phys_addr >= HIGH_MEMORY || phys_addr < LOW_MEM)
        return 0;
    to = *(unsigned long *)to_page;
    if (!(to & 1)) {
        if ((to = get_free_page()))
            *(unsigned long *)to_page = to | 7;
        else
            oom();
    }
    to &= 0xfffff000;
    to_page = to + ((address >> 10) & 0xffc);
    if (1 & *(unsigned long *)to_page)
        panic("try_to_share: to_page already exists");
    /* share them: write-protect */
    *(unsigned long *)from_page &= ~2;
    *(unsigned long *)to_page = *(unsigned long *)from_page;
    invalidate();
    phys_addr -= LOW_MEM;
    phys_addr >>= 12;
    mem_map[phys_addr]++;
    return 1;
}

/*
 * share_page() tries to find a process that could share a page with
 * the current one. Address is the address of the wanted page relative
 * to the current data space.
 *
 * We first check if it is at all feasible by checking executable->i_count.
 * It should be >1 if there are other tasks sharing this inode.
 */
static int share_page(unsigned long address) {
    struct task_struct **p;

    if (!current->executable)
        return 0;
    if (current->executable->i_count < 2)
        return 0;
    for (p = &LAST_TASK; p > &FIRST_TASK; --p) {
        if (!*p)
            continue;
        if (current == *p)
            continue;
        if ((*p)->executable != current->executable)
            continue;
        if (try_to_share(address, *p))
            return 1;
    }
    return 0;
}

void do_no_page(unsigned long error_code, unsigned long address) {
    int nr[4];
    unsigned long tmp;
    unsigned long page;
    int block, i;

    address &= 0xfffff000;
    tmp = address - current->start_code;
    if (!current->executable || tmp >= current->end_data) {
        get_empty_page(address);
        return;
    }
    if (share_page(tmp))
        return;
    if (!(page = get_free_page()))
        oom();
    /* remember that 1 block is used for header */
    block = 1 + tmp / BLOCK_SIZE;
    for (i = 0; i < 4; block++, i++) nr[i] = bmap(current->executable, block);
    bread_page(page, current->executable->i_dev, nr);
    i = tmp + 4096 - current->end_data;
    tmp = page + 4096;
    while (i-- > 0) {
        tmp--;
        *(char *)tmp = 0;
    }
    if (put_page(page, address))
        return;
    free_page(page);
    oom();
}

void mem_init(long start_mem, long end_mem) {
    int i;

    HIGH_MEMORY = end_mem;
    for (i = 0; i < PAGING_PAGES; i++) mem_map[i] = USED;
    i = MAP_NR(start_mem);
    end_mem -= start_mem;
    end_mem >>= 12;
    while (end_mem-- > 0) mem_map[i++] = 0;
}

void calc_mem(void) {
    int i, j, k, free = 0;
    long *pg_tbl;

    for (i = 0; i < PAGING_PAGES; i++)
        if (!mem_map[i])
            free++;
    printk("%d pages free (of %d)\n\r", free, PAGING_PAGES);
    for (i = 2; i < 1024; i++) {
        if (1 & pg_dir[i]) {
            pg_tbl = (long *)(0xfffff000 & pg_dir[i]);
            for (j = k = 0; j < 1024; j++)
                if (pg_tbl[j] & 1)
                    k++;
            printk("Pg-dir[%d] uses %d pages\n", i, k);
        }
    }
}
