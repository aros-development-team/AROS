/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_BITMAP_H_
#define _LINUX_BITMAP_H_

#include <string.h>
#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/find.h>
#include <linux/limits.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/bitmap-str.h>

#define BITMAP_FIRST_WORD_MASK(start)   (~0UL << ((start) & (BITS_PER_LONG - 1)))
#define BITMAP_LAST_WORD_MASK(nbits)    (~0UL >> (-(nbits) & (BITS_PER_LONG - 1)))

static inline void bitmap_zero(unsigned long *dst, unsigned int nbits)
{
    memset(dst, 0, BITS_TO_LONGS(nbits) * sizeof(unsigned long));
}
static inline void bitmap_fill(unsigned long *dst, unsigned int nbits)
{
    memset(dst, 0xff, BITS_TO_LONGS(nbits) * sizeof(unsigned long));
}
static inline void bitmap_copy(unsigned long *dst, const unsigned long *src, unsigned int nbits)
{
    memcpy(dst, src, BITS_TO_LONGS(nbits) * sizeof(unsigned long));
}
static inline void bitmap_set(unsigned long *map, unsigned int start, unsigned int nbits)
{
    while (nbits--)
        __set_bit(start++, map);
}
static inline void bitmap_clear(unsigned long *map, unsigned int start, unsigned int nbits)
{
    while (nbits--)
        __clear_bit(start++, map);
}
static inline bool bitmap_empty(const unsigned long *src, unsigned int nbits)
{
    return find_first_bit(src, nbits) == nbits;
}
static inline bool bitmap_full(const unsigned long *src, unsigned int nbits)
{
    return find_first_zero_bit(src, nbits) == nbits;
}
static inline unsigned int bitmap_weight(const unsigned long *src, unsigned int nbits)
{
    unsigned int i, w = 0;
    for (i = 0; i < nbits / BITS_PER_LONG; i++)
        w += hweight_long(src[i]);
    if (nbits % BITS_PER_LONG)
        w += hweight_long(src[i] & BITMAP_LAST_WORD_MASK(nbits));
    return w;
}
static inline bool bitmap_and(unsigned long *dst, const unsigned long *a, const unsigned long *b, unsigned int nbits)
{
    unsigned int i, n = BITS_TO_LONGS(nbits);
    unsigned long r = 0;
    for (i = 0; i < n; i++)
        r |= (dst[i] = a[i] & b[i]);
    return r != 0;
}
static inline void bitmap_or(unsigned long *dst, const unsigned long *a, const unsigned long *b, unsigned int nbits)
{
    unsigned int i, n = BITS_TO_LONGS(nbits);
    for (i = 0; i < n; i++)
        dst[i] = a[i] | b[i];
}
static inline void bitmap_xor(unsigned long *dst, const unsigned long *a, const unsigned long *b, unsigned int nbits)
{
    unsigned int i, n = BITS_TO_LONGS(nbits);
    for (i = 0; i < n; i++)
        dst[i] = a[i] ^ b[i];
}
static inline bool bitmap_andnot(unsigned long *dst, const unsigned long *a, const unsigned long *b, unsigned int nbits)
{
    unsigned int i, n = BITS_TO_LONGS(nbits);
    unsigned long r = 0;
    for (i = 0; i < n; i++)
        r |= (dst[i] = a[i] & ~b[i]);
    return r != 0;
}
static inline void bitmap_complement(unsigned long *dst, const unsigned long *src, unsigned int nbits)
{
    unsigned int i, n = BITS_TO_LONGS(nbits);
    for (i = 0; i < n; i++)
        dst[i] = ~src[i];
}
static inline bool bitmap_equal(const unsigned long *a, const unsigned long *b, unsigned int nbits)
{
    unsigned int i;
    for (i = 0; i < nbits / BITS_PER_LONG; i++)
        if (a[i] != b[i])
            return false;
    if (nbits % BITS_PER_LONG)
        if ((a[i] ^ b[i]) & BITMAP_LAST_WORD_MASK(nbits))
            return false;
    return true;
}
static inline bool bitmap_intersects(const unsigned long *a, const unsigned long *b, unsigned int nbits)
{
    unsigned int i;
    for (i = 0; i < nbits / BITS_PER_LONG; i++)
        if (a[i] & b[i])
            return true;
    if (nbits % BITS_PER_LONG)
        if ((a[i] & b[i]) & BITMAP_LAST_WORD_MASK(nbits))
            return true;
    return false;
}
static inline bool bitmap_subset(const unsigned long *a, const unsigned long *b, unsigned int nbits)
{
    unsigned int i;
    for (i = 0; i < nbits / BITS_PER_LONG; i++)
        if (a[i] & ~b[i])
            return false;
    if (nbits % BITS_PER_LONG)
        if ((a[i] & ~b[i]) & BITMAP_LAST_WORD_MASK(nbits))
            return false;
    return true;
}
static inline void bitmap_shift_right(unsigned long *dst, const unsigned long *src, unsigned int shift, unsigned int nbits)
{
    unsigned int i;
    for (i = 0; i < nbits; i++) {
        if (i + shift < nbits && test_bit(i + shift, src))
            __set_bit(i, dst);
        else
            __clear_bit(i, dst);
    }
}
static inline void bitmap_shift_left(unsigned long *dst, const unsigned long *src, unsigned int shift, unsigned int nbits)
{
    int i;
    for (i = (int)nbits - 1; i >= 0; i--) {
        if (i >= (int)shift && test_bit(i - shift, src))
            __set_bit(i, dst);
        else
            __clear_bit(i, dst);
    }
}
static inline unsigned long bitmap_find_next_zero_area(unsigned long *map, unsigned long size,
    unsigned long start, unsigned int nr, unsigned long align_mask)
{
    unsigned long index, end, i;
again:
    index = find_next_zero_bit(map, size, start);
    index = (index + align_mask) & ~align_mask;
    end = index + nr;
    if (end > size)
        return end;
    i = find_next_bit(map, end, index);
    if (i < end) {
        start = i + 1;
        goto again;
    }
    return index;
}
static inline unsigned long *bitmap_alloc(unsigned int nbits, gfp_t flags)
{
    return kmalloc_array(BITS_TO_LONGS(nbits), sizeof(unsigned long), flags);
}
static inline unsigned long *bitmap_zalloc(unsigned int nbits, gfp_t flags)
{
    return kcalloc(BITS_TO_LONGS(nbits), sizeof(unsigned long), flags);
}
static inline void bitmap_free(const unsigned long *bitmap)
{
    kfree(bitmap);
}
static inline void bitmap_from_u64(unsigned long *dst, u64 mask)
{
    dst[0] = mask & ULONG_MAX;
    if (sizeof(mask) > sizeof(unsigned long))
        dst[1] = mask >> 32;
}
static inline void bitmap_from_arr32(unsigned long *bitmap, const u32 *buf, unsigned int nbits)
{
    unsigned int i;
    bitmap_zero(bitmap, nbits);
    for (i = 0; i < nbits; i++)
        if (buf[i / 32] & (1U << (i % 32)))
            __set_bit(i, bitmap);
}
static inline void bitmap_to_arr32(u32 *buf, const unsigned long *bitmap, unsigned int nbits)
{
    unsigned int i;
    memset(buf, 0, BITS_TO_U32(nbits) * sizeof(u32));
    for (i = 0; i < nbits; i++)
        if (test_bit(i, bitmap))
            buf[i / 32] |= 1U << (i % 32);
}
#define bitmap_size(nbits)      (BITS_TO_LONGS(nbits) * sizeof(unsigned long))
#define BITMAP_FROM_U64(n)      ((unsigned long)(n))

#endif /* _LINUX_BITMAP_H_ */
