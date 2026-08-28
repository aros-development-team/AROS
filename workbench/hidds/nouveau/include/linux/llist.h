/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_LLIST_H_
#define _LINUX_LLIST_H_

#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#define LLIST_HEAD_INIT(name)   { NULL }
#define LLIST_HEAD(name)        struct llist_head name = LLIST_HEAD_INIT(name)
static inline void init_llist_head(struct llist_head *list) { list->first = NULL; }
static inline void init_llist_node(struct llist_node *node) { node->next = node; }
#define llist_entry(ptr, type, member)  container_of(ptr, type, member)
#define member_address_is_nonnull(ptr, member) ((IPTR)(ptr) + offsetof(typeof(*(ptr)), member) != 0)
#define llist_for_each(pos, node)       for ((pos) = (node); pos; (pos) = (pos)->next)
#define llist_for_each_safe(pos, n, node) for ((pos) = (node); (pos) && ((n) = (pos)->next, true); (pos) = (n))
#define llist_for_each_entry(pos, node, member) for ((pos) = llist_entry((node), typeof(*(pos)), member); member_address_is_nonnull(pos, member); (pos) = llist_entry((pos)->member.next, typeof(*(pos)), member))
#define llist_for_each_entry_safe(pos, n, node, member) for (pos = llist_entry((node), typeof(*pos), member); member_address_is_nonnull(pos, member) && (n = llist_entry(pos->member.next, typeof(*n), member), true); pos = n)
static inline bool llist_empty(const struct llist_head *head) { return READ_ONCE(head->first) == NULL; }
static inline struct llist_node *llist_next(struct llist_node *node) { return node->next; }
static inline bool llist_add_batch(struct llist_node *new_first, struct llist_node *new_last, struct llist_head *head)
{
    struct llist_node *first = head->first;
    do { new_last->next = first; } while (!try_cmpxchg(&head->first, &first, new_first));
    return !first;
}
static inline bool llist_add(struct llist_node *new, struct llist_head *head) { return llist_add_batch(new, new, head); }
static inline struct llist_node *llist_del_all(struct llist_head *head) { return xchg(&head->first, NULL); }
static inline struct llist_node *llist_del_first(struct llist_head *head)
{
    struct llist_node *entry, *next;
    entry = head->first;
    for (;;) {
        if (entry == NULL)
            return NULL;
        next = entry->next;
        if (try_cmpxchg(&head->first, &entry, next))
            return entry;
    }
}
static inline bool llist_on_list(const struct llist_node *node) { return node->next != node; }
static inline struct llist_node *llist_reverse_order(struct llist_node *head)
{
    struct llist_node *new_head = NULL;
    while (head) {
        struct llist_node *tmp = head;
        head = head->next;
        tmp->next = new_head;
        new_head = tmp;
    }
    return new_head;
}

#endif /* _LINUX_LLIST_H_ */
