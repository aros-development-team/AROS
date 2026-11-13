/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    INTERVAL_TREE_DEFINE over the plain red-black tree: nodes are kept in
    start order and a query walks forward from the leftmost node, stopping
    at the first node that starts after the query range.
*/

#ifndef _LINUX_INTERVAL_TREE_GENERIC_H_
#define _LINUX_INTERVAL_TREE_GENERIC_H_

#include <linux/rbtree.h>

#define INTERVAL_TREE_DEFINE(ITSTRUCT, ITRB, ITTYPE, ITSUBTREE,               \
                             ITSTART, ITLAST, ITSTATIC, ITPREFIX)            \
                                                                              \
ITSTATIC void ITPREFIX ## _insert(ITSTRUCT *node,                             \
                                  struct rb_root_cached *root)                \
{                                                                             \
    struct rb_node **link = &root->rb_root.rb_node, *rb_parent = NULL;        \
    ITTYPE start = ITSTART(node);                                             \
    bool leftmost = true;                                                     \
                                                                              \
    while (*link) {                                                           \
        ITSTRUCT *parent;                                                     \
        rb_parent = *link;                                                    \
        parent = rb_entry(rb_parent, ITSTRUCT, ITRB);                         \
        if (start < ITSTART(parent)) {                                        \
            link = &parent->ITRB.rb_left;                                     \
        } else {                                                              \
            link = &parent->ITRB.rb_right;                                    \
            leftmost = false;                                                 \
        }                                                                     \
    }                                                                         \
    rb_link_node(&node->ITRB, rb_parent, link);                               \
    rb_insert_color_cached(&node->ITRB, root, leftmost);                      \
}                                                                             \
                                                                              \
ITSTATIC void ITPREFIX ## _remove(ITSTRUCT *node,                             \
                                  struct rb_root_cached *root)                \
{                                                                             \
    rb_erase_cached(&node->ITRB, root);                                       \
}                                                                             \
                                                                              \
static inline ITSTRUCT *ITPREFIX ## _scan(struct rb_node *rb,                 \
                                          ITTYPE start, ITTYPE last)          \
{                                                                             \
    for (; rb; rb = rb_next(rb)) {                                            \
        ITSTRUCT *node = rb_entry(rb, ITSTRUCT, ITRB);                        \
        if (ITSTART(node) > last)                                             \
            return NULL;                                                      \
        if (ITLAST(node) >= start)                                            \
            return node;                                                      \
    }                                                                         \
    return NULL;                                                              \
}                                                                             \
                                                                              \
ITSTATIC ITSTRUCT *ITPREFIX ## _iter_first(struct rb_root_cached *root,       \
                                           ITTYPE start, ITTYPE last)         \
{                                                                             \
    return ITPREFIX ## _scan(rb_first_cached(root), start, last);             \
}                                                                             \
                                                                              \
ITSTATIC ITSTRUCT *ITPREFIX ## _iter_next(ITSTRUCT *node,                     \
                                          ITTYPE start, ITTYPE last)          \
{                                                                             \
    return ITPREFIX ## _scan(rb_next(&node->ITRB), start, last);              \
}

#endif
