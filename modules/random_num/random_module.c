#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/random.h>  // 随机数 API

static int __init random_init_module(void) {
    u32 rand;

    for (int i = 0; i < 10; i++) {
        // 获取一个 32 位随机数
        get_random_bytes(&rand, sizeof(rand));

        // 限制在 0-100 之间
        rand = rand % 100;

        printk(KERN_INFO "随机数: %u\n", rand);
    }

    return 0;
}

static void __exit random_exit_module(void) {
    printk(KERN_INFO "随机数模块卸载\n");
}

module_init(random_init_module);
module_exit(random_exit_module);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Linux 内核模块随机数生成示例");
