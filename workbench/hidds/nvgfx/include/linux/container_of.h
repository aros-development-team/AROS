/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_CONTAINER_OF_H_
#define _LINUX_CONTAINER_OF_H_

#include <linux/stddef.h>

#undef container_of
#define container_of(ptr, type, member) ({                              \
    void *__mptr = (void *)(ptr);                                       \
    ((type *)(__mptr - offsetof(type, member))); })
#define container_of_const(ptr, type, member) container_of(ptr, type, member)

#endif /* _LINUX_CONTAINER_OF_H_ */
