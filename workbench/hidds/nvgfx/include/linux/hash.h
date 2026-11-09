/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_HASH_H_
#define _LINUX_HASH_H_

#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/kernel.h>
#define GOLDEN_RATIO_32         0x61C88647
#define GOLDEN_RATIO_64         0x61C8864680B583EBull
static inline u32 __hash_32(u32 val)                     { return val * GOLDEN_RATIO_32; }
static inline u32 hash_32(u32 val, unsigned int bits)    { return __hash_32(val) >> (32 - bits); }
static inline u32 hash_64(u64 val, unsigned int bits)    { return (u32)((val * GOLDEN_RATIO_64) >> (64 - bits)); }
#define hash_long(val, bits)    ((sizeof(long) == 8) ? hash_64(val, bits) : hash_32(val, bits))
static inline u32 hash_ptr(const void *ptr, unsigned int bits) { return hash_long((unsigned long)ptr, bits); }
static inline u32 hash32_ptr(const void *ptr) { return (u32)(unsigned long)ptr; }

#endif /* _LINUX_HASH_H_ */
