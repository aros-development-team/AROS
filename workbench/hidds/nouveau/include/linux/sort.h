/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_SORT_H_
#define _LINUX_SORT_H_

#include <linux/types.h>
void sort(void *base, size_t num, size_t size, int (*cmp)(const void *, const void *), void (*swap)(void *, void *, int));
#define sort_r(b, n, s, c, sw, p) sort(b, n, s, (int (*)(const void *, const void *))(c), NULL)

#endif /* _LINUX_SORT_H_ */
