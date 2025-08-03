#ifndef COMMON_H
#define COMMON_H

#define MODULE_NAME    "kvm_ept_sample"
#define MODULE_PROT    0666

#define INFO(fmt, ...) pr_info(MODULE_NAME ": " fmt, ##__VA_ARGS__)

#ifndef __KERNEL__

#ifdef NO_ASSERT
#define assert(x)
#else
#include <assert.h>
#endif

#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#define ERROR0(ret, msg)                                                        \
    ({                                                                          \
        fprintf(stderr, "[<%s> @ %s: %d]: ", __FUNCTION__, __FILE__, __LINE__); \
        fprintf(stderr, msg "\n");                                              \
        return (ret);                                                           \
    })

#define ERROR1(ret, msg, arg1)                                                  \
    ({                                                                          \
        fprintf(stderr, "[<%s> @ %s: %d]: ", __FUNCTION__, __FILE__, __LINE__); \
        fprintf(stderr, msg "\n", arg1);                                        \
        return (ret);                                                           \
    })

#define ERROR2(ret, msg, arg1, arg2)                                            \
    ({                                                                          \
        fprintf(stderr, "[<%s> @ %s: %d]: ", __FUNCTION__, __FILE__, __LINE__); \
        fprintf(stderr, msg "\n", arg1, arg2);                                  \
        return (ret);                                                           \
    })

#define ERROR3(ret, msg, arg1, arg2, arg3)                                      \
    ({                                                                          \
        fprintf(stderr, "[<%s> @ %s: %d]: ", __FUNCTION__, __FILE__, __LINE__); \
        fprintf(stderr, msg "\n", arg1, arg2, arg3);                            \
        return (ret);                                                           \
    })

#else

#include <linux/module.h>
#include <linux/types.h>

#ifdef NO_ASSERT
#define assert(x)
#else
#define assert(x)                                                          \
    ({                                                                     \
        if (!(x))                                                          \
            panic("assert('%s')failed in %s: %d", #x, __FILE__, __LINE__); \
    })
#endif

#define ERROR0(ret, msg)                                                        \
    ({                                                                          \
        printk(KERN_ERR "[<%s> @ %s: %d]: ", __FUNCTION__, __FILE__, __LINE__); \
        printk(KERN_ERR msg "\n");                                              \
        return (ret);                                                           \
    })

#define ERROR1(ret, msg, arg1)                                                  \
    ({                                                                          \
        printk(KERN_ERR "[<%s> @ %s: %d]: ", __FUNCTION__, __FILE__, __LINE__); \
        printk(KERN_ERR msg "\n", arg1);                                        \
        return (ret);                                                           \
    })

#define ERROR2(ret, msg, arg1, arg2)                                            \
    ({                                                                          \
        printk(KERN_ERR "[<%s> @ %s: %d]: ", __FUNCTION__, __FILE__, __LINE__); \
        printk(KERN_ERR msg "\n", arg1, arg2);                                  \
        return (ret);                                                           \
    })

#define ERROR3(ret, msg, arg1, arg2, arg3)                                      \
    ({                                                                          \
        printk(KERN_ERR "[<%s> @ %s: %d]: ", __FUNCTION__, __FILE__, __LINE__); \
        printk(KERN_ERR msg "\n", arg1, arg2, arg3);                            \
        return (ret);                                                           \
    })

#endif

#ifndef likely
#define likely(x) __builtin_expect(!!(x), 1)
#endif

#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

#ifndef MIN2
#define MIN2(a, b)         \
    ({                     \
        typeof(a) x = (a); \
        typeof(b) y = (b); \
        x < y ? x : y;     \
    })
#endif

#ifndef MAX2
#define MAX2(a, b)         \
    ({                     \
        typeof(a) x = (a); \
        typeof(b) y = (b); \
        x > y ? x : y;     \
    })
#endif

#endif
