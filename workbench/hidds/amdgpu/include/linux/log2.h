/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_LOG2_H_
#define _LINUX_LOG2_H_

#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/kernel.h>

static inline __attribute__((__const__)) bool is_power_of_2(unsigned long n)
{
    return (n != 0 && ((n & (n - 1)) == 0));
}
static inline __attribute__((__const__)) int __ilog2_u32(u32 n) { return fls(n) - 1; }
static inline __attribute__((__const__)) int __ilog2_u64(u64 n) { return fls64(n) - 1; }
#define ilog2(n) ((sizeof(n) <= 4) ? __ilog2_u32((u32)(n)) : __ilog2_u64((u64)(n)))
#define const_ilog2(n) ilog2(n)
static inline unsigned long __roundup_pow_of_two(unsigned long n) { return 1UL << fls_long(n - 1); }
static inline unsigned long __rounddown_pow_of_two(unsigned long n) { return 1UL << (fls_long(n) - 1); }
#define roundup_pow_of_two(n)   __roundup_pow_of_two(n)
#define rounddown_pow_of_two(n) __rounddown_pow_of_two(n)
#define order_base_2(n)         ((n) > 1 ? ilog2((n) - 1) + 1 : 0)
#define bits_per(n)             ((n) ? ilog2(n) + 1 : 1)
static inline int get_order(unsigned long size)
{
    size--;
    size >>= 12;
    return size ? fls_long(size) : 0;
}

#endif /* _LINUX_LOG2_H_ */
