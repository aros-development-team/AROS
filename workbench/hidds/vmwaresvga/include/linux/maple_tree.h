/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_MAPLE_TREE_H_
#define _LINUX_MAPLE_TREE_H_

#include <linux/spinlock.h>

/* only the shape is needed: the users of this tree are not compiled */
struct maple_tree {
    spinlock_t ma_lock;
    unsigned int ma_flags;
    void *ma_root;
};
#define MT_FLAGS_ALLOC_RANGE    0x01
#define MT_FLAGS_LOCK_EXTERN    0x300
#define mt_init_flags(mt, f)    do { (mt)->ma_flags = (f); (mt)->ma_root = NULL; } while (0)
#define mt_init(mt)             mt_init_flags(mt, 0)
#define mtree_destroy(mt)       do { } while (0)
#define mtree_empty(mt)         ((mt)->ma_root == NULL)

#endif
