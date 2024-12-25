#include "interact.h"

#include "common.h"

static void *alloc_page_for_queue(void *privdata) {
    return (void *)__get_free_page(GFP_ATOMIC);
}

static void free_page_for_queue(void *page, void *privdata) {
    free_page((unsigned long)page);
}

int interact_open(struct inode *inode, struct file *file) {
    int ret;
    struct interact *interact;
    assert(!file->private_data);
    if (!(interact = kzalloc(sizeof(struct interact), GFP_KERNEL)))
        ERROR0(-ENOMEM, "kzalloc(sizeof(struct interact), GFP_KERNEL) failed");
    sema_init(&(interact->file_lock), 1);
    if ((ret = queue_init(&(interact->queue),
                          sizeof(struct interact_sample),
                          PAGE_SIZE,
                          alloc_page_for_queue,
                          free_page_for_queue,
                          NULL))) {
        kfree(interact);
        ERROR0(ret, "queue_init(&(interact->queue), ...) failed");
    }
    spin_lock_init(&(interact->queue_lock));
    assert(!interact->sampler.privdata);
    assert(!file->private_data);
    file->private_data = interact;
    INFO("Opened /proc/%s\n", MODULE_NAME);
    return 0;
}

/**
 * @brief handle ept sample to human readable format
 *
 * @param gpa
 * @param xwr
 * @param privdata
 */
static void handle_ept_sample(unsigned long gpa, int xwr, void *privdata) {
    struct interact *interact = privdata;
    struct interact_sample *sample;
    assert(interact);
    if (!spin_trylock(&(interact->queue_lock)))
        return;
    if (interact->queue.length >= INTERACT_MAX_BUFFERED_SAMPLES || !(sample = queue_add(&(interact->queue)))) {
        spin_unlock(&(interact->queue_lock));
        return;
    }
    sample->gfn = gpa >> PAGE_SHIFT;
    sample->xwr = xwr;
    spin_unlock(&(interact->queue_lock));
}

static int handle_cmd_init(struct interact *interact, pid_t pid) {
    int ret;
    if (interact->sampler.privdata)
        ERROR0(-EINVAL, "this fd has been inited already");
    if ((ret = sampler_init(&(interact->sampler), pid, handle_ept_sample, interact))) {
        assert(!interact->sampler.privdata);
        ERROR1(ret, "sampler_init(&(interact->sampler), %d, ...) failed", pid);
    }
    assert(interact->sampler.privdata == interact);
    return 0;
}

static int handle_cmd_set_prot(struct interact *interact, int xwr) {
    if (!interact->sampler.privdata)
        ERROR0(-EINVAL, "use KVM_EPT_SAMPLE_CMD_INIT to init first");
    sampler_set_prot(&(interact->sampler), xwr);
    return 0;
}

static int handle_cmd_set_freq(struct interact *interact, unsigned long freq) {
    if (!interact->sampler.privdata)
        ERROR0(-EINVAL, "use KVM_EPT_SAMPLE_CMD_INIT to init first");
    sampler_set_freq(&(interact->sampler), freq);
    return 0;
}

static int handle_cmd_get_memslots(struct interact *interact, struct interact_get_memslots *__user param) {
    struct kvm_memslots *slots;
    struct kvm_memory_slot *ms;
    int bkt;
    // size_t slot_count;
    struct interact_memslot *__user user_memslots;
    if (!param)
        ERROR0(-EINVAL, "param <param = NULL> is invalid");
    if (!interact->sampler.privdata)
        ERROR0(-EINVAL, "this fd has not been inited yet");

    // slot_count = kvm_memslots->used_slots;
    if (copy_from_user(&user_memslots, &(param->memslots), sizeof(void *)))
        ERROR1(-EIO, "copy_from_user(..., %p, sizeof(void*)) failed", &(param->memslots));

    int count = 0;
    int capacity;
    if (get_user(capacity, &(param->capacity)))
        ERROR1(-EIO, "get_user(..., %p) failed", &(param->capacity));

    slots = kvm_memslots(interact->sampler.kvm);
    if (!slots || kvm_memslots_empty(slots))
        return -EINVAL;
    kvm_for_each_memslot(ms, bkt, slots) {
        if (count >= capacity)
            break;
        struct interact_memslot *__user dst = user_memslots + count;
        if (put_user(ms->base_gfn << PAGE_SHIFT, &(dst->gpa)))
            ERROR1(-EIO, "put_user(..., %p) failed", &(dst->gpa));
        if (put_user(ms->userspace_addr, &(dst->hva)))
            ERROR1(-EIO, "put_user(..., %p) failed", &(dst->hva));
        if (put_user(ms->npages, &(dst->page_count)))
            ERROR1(-EIO, "put_user(..., %p) failed", &(dst->page_count));
        count++;
    }
    
    INFO("count = %d\n", count);
    if (put_user(count, &(param->count)))
        ERROR1(-EIO, "put_user(..., %p) failed", &(param->count));

    return 0;
}

static int handle_cmd_deinit(struct interact *interact, int check) {
    if (!interact->sampler.privdata) {
        if (check)
            ERROR0(-EINVAL, "this fd has not been inited yet");
        else
            return 0;
    }
    sampler_deinit(&(interact->sampler));
    interact->sampler.privdata = NULL;
    return 0;
}

long interact_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    int ret;
    struct interact *interact = file->private_data;
    assert(interact);
    down(&(interact->file_lock));
    switch (cmd) {
        case KVM_EPT_SAMPLE_CMD_INIT:
            ret = handle_cmd_init(interact, (pid_t)arg);
            break;
        case KVM_EPT_SAMPLE_CMD_SET_PROT:
            ret = handle_cmd_set_prot(interact, (int)arg);
            break;
        case KVM_EPT_SAMPLE_CMD_SET_FREQ:
            ret = handle_cmd_set_freq(interact, arg);
            break;
        case KVM_EPT_SAMPLE_CMD_GET_MEMSLOTS:
            ret = handle_cmd_get_memslots(interact, (void *)arg);
            break;
        case KVM_EPT_SAMPLE_CMD_DEINIT:
            ret = handle_cmd_deinit(interact, 1);
            break;
        default:
            up(&(interact->file_lock));
            ERROR1(-EINVAL, "unknown command: %u\n", cmd);
            break;
    }
    up(&(interact->file_lock));
    return ret;
}

// get sample data from the queue
ssize_t interact_read(struct file *file, char *buffer, size_t capacity, loff_t *offset) {
    struct interact *interact = file->private_data;
    size_t size = 0;
    if (!buffer)
        return 0;
    assert(interact);
    down(&(interact->file_lock));
    while (size < capacity) {
        struct interact_sample *sample;
        spin_lock(&(interact->queue_lock));
        sample = queue_take(&(interact->queue));
        spin_unlock(&(interact->queue_lock));
        if (!sample)
            break;
        if (copy_to_user(buffer + size, sample, sizeof(struct interact_sample))) {
            up(&(interact->file_lock));
            ERROR1(-EIO, "copy_to_user(%p, sample, sizeof(struct interact_sample)) failed", buffer + size);
        }
        size += sizeof(struct interact_sample);
    }
    up(&(interact->file_lock));
    return size;
}

int interact_release(struct inode *inode, struct file *file) {
    struct interact *interact = file->private_data;
    assert(interact);
    handle_cmd_deinit(interact, 0);
    queue_deinit(&(interact->queue), NULL);
    kfree(interact);
    file->private_data = NULL;
    INFO("Release /proc/%s\n", MODULE_NAME);
    return 0;
}
