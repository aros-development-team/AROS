/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_LIST_SORT_H_
#define _LINUX_LIST_SORT_H_

#include <linux/list.h>
typedef int (*list_cmp_func_t)(void *, const struct list_head *, const struct list_head *);
void list_sort(void *priv, struct list_head *head, list_cmp_func_t cmp);

#endif /* _LINUX_LIST_SORT_H_ */
