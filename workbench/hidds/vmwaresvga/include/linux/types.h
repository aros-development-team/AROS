/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_TYPES_H_
#define _LINUX_TYPES_H_

#include <aros/config.h>
#include <exec/types.h>
#include <aros/macros.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <errno.h>

#define __user
#define __iomem
#define __force
#define __must_check
#define __rcu
#define __percpu
#define __kernel
#define __bitwise
#define __poll_t        unsigned int
#define __private

typedef unsigned char           u8;
typedef signed char             s8;
typedef unsigned short          u16;
typedef signed short            s16;
typedef unsigned int            u32;
typedef signed int              s32;
/* u64 follows uint64_t so that the two stay interchangeable, as they are
   in the kernel; on 64-bit AROS that makes it unsigned long */
typedef uint64_t                u64;
typedef int64_t                 s64;

typedef u8                      __u8;
typedef s8                      __s8;
typedef u16                     __u16;
typedef s16                     __s16;
typedef u32                     __u32;
typedef s32                     __s32;
typedef u64                     __u64;
typedef s64                     __s64;

typedef u16                     __le16;
typedef u16                     __be16;
typedef u32                     __le32;
typedef u32                     __be32;
typedef u64                     __le64;
typedef u64                     __be64;
typedef u16                     __sum16;
typedef u32                     __wsum;

typedef u64                     dma_addr_t;
typedef u64                     phys_addr_t;
typedef phys_addr_t             resource_size_t;
typedef IPTR                    __kernel_size_t;
typedef LONG                    __kernel_ssize_t;
typedef s64                     loff_t;
typedef IPTR                    pgoff_t;
typedef unsigned long           kernel_ulong_t;
typedef unsigned int            gfp_t;
typedef unsigned int            fmode_t;
typedef u32                     umode_t;
typedef s64                     ktime_t;
typedef s64                     time64_t;

typedef struct { LONG counter; } atomic_t;
typedef struct { QUAD counter; } atomic64_t;
typedef atomic64_t              atomic_long_t;

typedef struct { unsigned long pgprot; } pgprot_t;

struct list_head {
    struct list_head *next, *prev;
};

struct hlist_head {
    struct hlist_node *first;
};

struct hlist_node {
    struct hlist_node *next, **pprev;
};

struct llist_node {
    struct llist_node *next;
};

struct llist_head {
    struct llist_node *first;
};

struct rcu_head {
    void *next;
    void (*func)(struct rcu_head *);
};

/* names that Linux headers use in prototypes before defining them */
struct file;
struct inode;
struct vfsmount;
struct folio;
struct poll_table_struct;
struct vm_area_struct;
struct vm_fault;
struct address_space;
struct seq_file;
struct dentry;
struct device;
struct module;
struct task_struct;
struct sg_table;
struct scatterlist;
struct page;
struct kobject;
struct attribute_group;
struct dma_buf;
struct dma_buf_attachment;
struct dma_fence;
struct dma_resv;
struct ww_acquire_ctx;
struct completion;
struct workqueue_struct;
struct work_struct;
struct mutex;
struct kthread_work;
struct kthread_worker;
struct dev_pm_ops;
struct pci_dev;
struct pci_bus;
struct i2c_adapter;
struct i2c_client;
struct firmware;
struct notifier_block;
struct backlight_device;
struct dentry;
struct kmsg_dumper;
struct hrtimer;
struct timer_list;
struct rb_node;
struct rb_root;

/*
 * A page is identified by the (page-aligned) address of the memory it
 * covers, and the type is sized to match so that page pointer arithmetic
 * walks the pages themselves. Nothing dereferences a struct page.
 */
struct page {
    unsigned char bytes[4096];
};

typedef unsigned int            irqreturn_t;
#define IRQ_NONE                0
#define IRQ_HANDLED             1
#define IRQ_WAKE_THREAD         2

#define U8_MAX                  ((u8)~0U)
#define S8_MAX                  ((s8)(U8_MAX >> 1))
#define S8_MIN                  ((s8)(-S8_MAX - 1))
#define U16_MAX                 ((u16)~0U)
#define S16_MAX                 ((s16)(U16_MAX >> 1))
#define S16_MIN                 ((s16)(-S16_MAX - 1))
#define U32_MAX                 ((u32)~0U)
#define U32_MIN                 ((u32)0)
#define S32_MAX                 ((s32)(U32_MAX >> 1))
#define S32_MIN                 ((s32)(-S32_MAX - 1))
#define U64_MAX                 ((u64)~0ULL)
#define S64_MAX                 ((s64)(U64_MAX >> 1))
#define S64_MIN                 ((s64)(-S64_MAX - 1))
#ifndef SIZE_MAX
#define SIZE_MAX                (~(size_t)0)
#endif
#ifndef SSIZE_MAX
#define SSIZE_MAX               ((ssize_t)(SIZE_MAX >> 1))
#endif
#ifndef PHYS_ADDR_MAX
#define PHYS_ADDR_MAX           (~(phys_addr_t)0)
#endif

#define S8_C(x)                 x
#define U8_C(x)                 x ## U
#define S16_C(x)                x
#define U16_C(x)                x ## U
#define S32_C(x)                x
#define U32_C(x)                x ## U
#define S64_C(x)                x ## LL
#define U64_C(x)                x ## ULL

#define DECLARE_BITMAP(name, bits) \
    unsigned long name[(((bits) + (8 * sizeof(long)) - 1) / (8 * sizeof(long)))]

#endif /* _LINUX_TYPES_H_ */
