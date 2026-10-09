#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/mm_types.h>
#include <linux/mmap_lock.h>
#include <linux/srcu.h>
#include <linux/interval_tree.h>
/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_MMU_NOTIFIER_H_
#define _LINUX_MMU_NOTIFIER_H_

struct mmu_notifier { int dummy; };
struct mmu_notifier_ops;
struct mmu_notifier_range;
struct mmu_interval_notifier;
#define mmu_notifier_range_blockable(r) true
#define mmu_notifier_synchronize() do { } while (0)

#endif /* _LINUX_MMU_NOTIFIER_H_ */
