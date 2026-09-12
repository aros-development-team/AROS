/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_RANDOM_H_
#define _LINUX_RANDOM_H_

#include <linux/types.h>
#include <linux/list.h>
#include <linux/once.h>
u32 get_random_u32(void);
#define get_random_u64()        (((u64)get_random_u32() << 32) | get_random_u32())
#define get_random_bytes(b, n)  do { u8 *__b = (u8 *)(b); size_t __i; for (__i = 0; __i < (n); __i++) __b[__i] = get_random_u32(); } while (0)
#define get_random_u32_below(c) (get_random_u32() % (c))
#define prandom_u32_max(c)      get_random_u32_below(c)

#endif /* _LINUX_RANDOM_H_ */
