/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_MAPLE_TREE_H_
#define _LINUX_MAPLE_TREE_H_

#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/gfp.h>

/*
 * A map of index ranges to entries: the ranges are kept on a list in
 * index order, enough for the handful of sparse regions a VM has. The
 * state of a walk is the range last looked at.
 */
struct maple_tree {
    spinlock_t ma_lock;
    unsigned int ma_flags;
    struct list_head ma_ranges;
};

struct ma_state {
    struct maple_tree *tree;
    unsigned long index;
    unsigned long last;
    bool active;        /* index/last are the range the last find returned */
};

#define MT_FLAGS_ALLOC_RANGE    0x01
#define MT_FLAGS_LOCK_EXTERN    0x300

#define MA_STATE(name, mt, first, end) \
    struct ma_state name = { .tree = (mt), .index = (first), .last = (end), .active = false }

#define mt_init_flags(mt, f)    do { (mt)->ma_flags = (f); spin_lock_init(&(mt)->ma_lock); INIT_LIST_HEAD(&(mt)->ma_ranges); } while (0)
#define mt_init(mt)             mt_init_flags(mt, 0)
#define mt_set_external_lock(mt, lock) do { } while (0)
#define mtree_empty(mt)         list_empty(&(mt)->ma_ranges)
#define mtree_destroy(mt)       __mt_destroy(mt)

void *mas_walk(struct ma_state *mas);
int mas_store_gfp(struct ma_state *mas, void *entry, gfp_t gfp);
void *mas_erase(struct ma_state *mas);
void *mas_find(struct ma_state *mas, unsigned long max);
void __mt_destroy(struct maple_tree *mt);

#define mas_for_each(mas, entry, max) \
    while (((entry) = mas_find((mas), (max))) != NULL)

#endif /* _LINUX_MAPLE_TREE_H_ */
