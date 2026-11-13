/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_RCULIST_H_
#define _LINUX_RCULIST_H_

#include <linux/rcupdate.h>
#include <linux/list.h>

#define list_add_rcu(n, h)              list_add(n, h)
#define list_add_tail_rcu(n, h)         list_add_tail(n, h)
#define list_del_rcu(e)                 list_del(e)
#define list_del_init_rcu(e)            list_del_init(e)
#define list_replace_rcu(o, n)          list_replace(o, n)
#define list_for_each_entry_rcu(pos, head, member, ...) list_for_each_entry(pos, head, member)
#define list_for_each_entry_continue_rcu(pos, head, member) list_for_each_entry_continue(pos, head, member)
#define list_for_each_entry_lockless(pos, head, member) list_for_each_entry(pos, head, member)
#define list_entry_rcu(ptr, type, member) list_entry(ptr, type, member)
#define list_first_or_null_rcu(ptr, type, member) list_first_entry_or_null(ptr, type, member)
#define list_next_or_null_rcu(head, ptr, type, member) ({ struct list_head *__n = (ptr)->next; (__n != (head)) ? list_entry(__n, type, member) : NULL; })
#define hlist_add_head_rcu(n, h)        hlist_add_head(n, h)
#define hlist_del_rcu(n)                hlist_del(n)
#define hlist_del_init_rcu(n)           hlist_del_init(n)
#define hlist_for_each_entry_rcu(pos, head, member, ...) hlist_for_each_entry(pos, head, member)

#endif /* _LINUX_RCULIST_H_ */
