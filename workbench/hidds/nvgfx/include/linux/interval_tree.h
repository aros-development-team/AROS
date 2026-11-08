/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_INTERVAL_TREE_H_
#define _LINUX_INTERVAL_TREE_H_

#include <linux/rbtree.h>
#include <linux/interval_tree_generic.h>
struct interval_tree_node {
    struct rb_node rb;
    unsigned long start;
    unsigned long last;
    unsigned long __subtree_last;
};

#endif /* _LINUX_INTERVAL_TREE_H_ */
