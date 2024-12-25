#include <linux/fs.h>
#include <linux/module.h>
#include <linux/proc_fs.h>

#include "common.h"
#include "interact.h"

static struct proc_dir_entry *proc_entry;

static struct proc_ops fops = {
    .proc_open = interact_open,
    .proc_ioctl = interact_ioctl,
    .proc_read = interact_read,
    .proc_release = interact_release,
};

static int __init init(void) {
    proc_entry = proc_create(MODULE_NAME, MODULE_PROT, NULL, &fops);
    if (proc_entry == NULL) {
        pr_err("Failed to create /proc/%s\n", MODULE_NAME);
        return -EIO;
    }

    INFO("Created /proc/%s\n", MODULE_NAME);
    return 0;
}

static void __exit cleanup(void) {
    if (proc_entry)
        remove_proc_entry(MODULE_NAME, NULL);
    INFO("Removed /proc/%s\n", MODULE_NAME);
}

module_init(init);
module_exit(cleanup);

MODULE_LICENSE("GPL");