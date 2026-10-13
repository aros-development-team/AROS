/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_SWAB_H_
#define _LINUX_SWAB_H_

#include <linux/types.h>
#define swab16(x)       __builtin_bswap16(x)
#define swab32(x)       __builtin_bswap32(x)
#define swab64(x)       __builtin_bswap64(x)
#define __swab16(x)     __builtin_bswap16(x)
#define __swab32(x)     __builtin_bswap32(x)
#define __swab64(x)     __builtin_bswap64(x)
static inline void swab16s(u16 *p) { *p = swab16(*p); }
static inline void swab32s(u32 *p) { *p = swab32(*p); }
static inline void swab64s(u64 *p) { *p = swab64(*p); }

#endif /* _LINUX_SWAB_H_ */
