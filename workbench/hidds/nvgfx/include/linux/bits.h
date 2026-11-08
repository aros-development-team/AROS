/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_BITS_H_
#define _LINUX_BITS_H_

#include <linux/const.h>

#if defined(__LP64__) || defined(_LP64) || (__SIZEOF_LONG__ == 8)
#define BITS_PER_LONG           64
#else
#define BITS_PER_LONG           32
#endif
#define BITS_PER_LONG_LONG      64

#define BIT(nr)                 (1UL << (nr))
#define BIT_ULL(nr)             (1ULL << (nr))
#define GENMASK(h, l)           (((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_LONG - 1 - (h))))
#define GENMASK_ULL(h, l)       (((~0ULL) - (1ULL << (l)) + 1) & (~0ULL >> (BITS_PER_LONG_LONG - 1 - (h))))
#define GENMASK_U8(h, l)        ((u8)GENMASK(h, l))
#define GENMASK_U16(h, l)       ((u16)GENMASK(h, l))
#define GENMASK_U32(h, l)       ((u32)GENMASK_ULL(h, l))
#define GENMASK_U64(h, l)       GENMASK_ULL(h, l)
#define __GENMASK(h, l)         GENMASK(h, l)
#define __GENMASK_ULL(h, l)     GENMASK_ULL(h, l)
#define BIT_U8(n)               ((u8)BIT(n))
#define BIT_U16(n)              ((u16)BIT(n))
#define BIT_U32(n)              ((u32)BIT(n))
#define BIT_U64(n)              ((u64)BIT_ULL(n))

#endif /* _LINUX_BITS_H_ */
