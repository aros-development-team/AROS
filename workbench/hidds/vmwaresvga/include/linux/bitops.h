/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_BITOPS_H_
#define _LINUX_BITOPS_H_

#include <linux/types.h>
#include <linux/compiler.h>
#include <linux/bits.h>
#include <linux/typecheck.h>
#include <linux/const.h>

#define BITS_PER_BYTE           8
#define BITS_PER_TYPE(type)     (sizeof(type) * BITS_PER_BYTE)
#define BITS_TO_LONGS(nr)       (((nr) + BITS_PER_LONG - 1) / BITS_PER_LONG)
#define BITS_TO_U64(nr)         (((nr) + 63) / 64)
#define BITS_TO_U32(nr)         (((nr) + 31) / 32)
#define BITS_TO_BYTES(nr)       (((nr) + 7) / 8)
#define BYTES_TO_BITS(nb)       ((nb) * BITS_PER_BYTE)
#define BIT_MASK(nr)            (1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)            ((nr) / BITS_PER_LONG)
#define BIT_ULL_MASK(nr)        (1ULL << ((nr) % 64))
#define BIT_ULL_WORD(nr)        ((nr) / 64)

/* --- word-wide helpers ------------------------------------------------- */

static inline int fls(unsigned int x)
{
    return x ? (int)(32 - __builtin_clz(x)) : 0;
}
static inline int fls64(u64 x)
{
    return x ? (int)(64 - __builtin_clzll(x)) : 0;
}
static inline int flsl(unsigned long x)
{
    return x ? (int)(BITS_PER_LONG - __builtin_clzl(x)) : 0;
}
static inline int ffs(int x)
{
    return __builtin_ffs(x);
}
static inline int ffsl(long x)
{
    return __builtin_ffsl(x);
}
static inline int ffsll(long long x)
{
    return __builtin_ffsll(x);
}
static inline unsigned long __ffs(unsigned long x)
{
    return (unsigned long)__builtin_ctzl(x);
}
static inline unsigned long __fls(unsigned long x)
{
    return BITS_PER_LONG - 1 - (unsigned long)__builtin_clzl(x);
}
static inline unsigned long ffz(unsigned long x)
{
    return __ffs(~x);
}
static inline unsigned int __ffs64(u64 x)
{
    return (unsigned int)__builtin_ctzll(x);
}
static inline unsigned int fls_long(unsigned long x)
{
    return (unsigned int)flsl(x);
}
static inline unsigned int hweight8(unsigned int w)  { return (unsigned int)__builtin_popcount(w & 0xff); }
static inline unsigned int hweight16(unsigned int w) { return (unsigned int)__builtin_popcount(w & 0xffff); }
static inline unsigned int hweight32(unsigned int w) { return (unsigned int)__builtin_popcount(w); }
static inline unsigned int hweight64(u64 w)          { return (unsigned int)__builtin_popcountll(w); }
static inline unsigned long hweight_long(unsigned long w) { return (unsigned long)__builtin_popcountl(w); }

static inline int get_count_order(unsigned int count)
{
    if (count == 0)
        return -1;
    return fls(count - 1);
}
static inline int get_count_order_long(unsigned long l)
{
    if (l == 0UL)
        return -1;
    return (int)fls_long(l - 1UL);
}
static inline int get_bitmask_order(unsigned int count)
{
    return fls(count);
}

static inline u64 rol64(u64 word, unsigned int shift) { return (word << (shift & 63)) | (word >> ((-shift) & 63)); }
static inline u64 ror64(u64 word, unsigned int shift) { return (word >> (shift & 63)) | (word << ((-shift) & 63)); }
static inline u32 rol32(u32 word, unsigned int shift) { return (word << (shift & 31)) | (word >> ((-shift) & 31)); }
static inline u32 ror32(u32 word, unsigned int shift) { return (word >> (shift & 31)) | (word << ((-shift) & 31)); }
static inline u16 rol16(u16 word, unsigned int shift) { return (word << (shift & 15)) | (word >> ((-shift) & 15)); }
static inline u16 ror16(u16 word, unsigned int shift) { return (word >> (shift & 15)) | (word << ((-shift) & 15)); }
static inline u8 rol8(u8 word, unsigned int shift)    { return (word << (shift & 7)) | (word >> ((-shift) & 7)); }
static inline u8 ror8(u8 word, unsigned int shift)    { return (word >> (shift & 7)) | (word << ((-shift) & 7)); }

static inline s32 sign_extend32(u32 value, int index)
{
    u8 shift = 31 - index;
    return (s32)(value << shift) >> shift;
}
static inline s64 sign_extend64(u64 value, int index)
{
    u8 shift = 63 - index;
    return (s64)(value << shift) >> shift;
}

/* --- atomic bit ops on unsigned long arrays ---------------------------- */

static inline void set_bit(long nr, volatile unsigned long *addr)
{
    __atomic_fetch_or(&addr[BIT_WORD(nr)], BIT_MASK(nr), __ATOMIC_SEQ_CST);
}
static inline void clear_bit(long nr, volatile unsigned long *addr)
{
    __atomic_fetch_and(&addr[BIT_WORD(nr)], ~BIT_MASK(nr), __ATOMIC_SEQ_CST);
}
static inline void change_bit(long nr, volatile unsigned long *addr)
{
    __atomic_fetch_xor(&addr[BIT_WORD(nr)], BIT_MASK(nr), __ATOMIC_SEQ_CST);
}
static inline int test_and_set_bit(long nr, volatile unsigned long *addr)
{
    return (__atomic_fetch_or(&addr[BIT_WORD(nr)], BIT_MASK(nr), __ATOMIC_SEQ_CST) & BIT_MASK(nr)) != 0;
}
static inline int test_and_clear_bit(long nr, volatile unsigned long *addr)
{
    return (__atomic_fetch_and(&addr[BIT_WORD(nr)], ~BIT_MASK(nr), __ATOMIC_SEQ_CST) & BIT_MASK(nr)) != 0;
}
static inline int test_and_change_bit(long nr, volatile unsigned long *addr)
{
    return (__atomic_fetch_xor(&addr[BIT_WORD(nr)], BIT_MASK(nr), __ATOMIC_SEQ_CST) & BIT_MASK(nr)) != 0;
}
static inline int test_bit(long nr, const volatile unsigned long *addr)
{
    return (addr[BIT_WORD(nr)] >> (nr % BITS_PER_LONG)) & 1;
}
static inline void __set_bit(long nr, volatile unsigned long *addr)
{
    addr[BIT_WORD(nr)] |= BIT_MASK(nr);
}
static inline void __clear_bit(long nr, volatile unsigned long *addr)
{
    addr[BIT_WORD(nr)] &= ~BIT_MASK(nr);
}
static inline void __change_bit(long nr, volatile unsigned long *addr)
{
    addr[BIT_WORD(nr)] ^= BIT_MASK(nr);
}
static inline int __test_and_set_bit(long nr, volatile unsigned long *addr)
{
    unsigned long old = addr[BIT_WORD(nr)];
    addr[BIT_WORD(nr)] = old | BIT_MASK(nr);
    return (old & BIT_MASK(nr)) != 0;
}
static inline int __test_and_clear_bit(long nr, volatile unsigned long *addr)
{
    unsigned long old = addr[BIT_WORD(nr)];
    addr[BIT_WORD(nr)] = old & ~BIT_MASK(nr);
    return (old & BIT_MASK(nr)) != 0;
}
#define set_bit_unlock(nr, addr)            clear_bit(nr, addr)
#define clear_bit_unlock(nr, addr)          clear_bit(nr, addr)
#define __clear_bit_unlock(nr, addr)        __clear_bit(nr, addr)
#define test_and_set_bit_lock(nr, addr)     test_and_set_bit(nr, addr)
#define test_bit_acquire(nr, addr)          test_bit(nr, addr)
#define assign_bit(nr, addr, value)         do { if (value) set_bit(nr, addr); else clear_bit(nr, addr); } while (0)
#define __assign_bit(nr, addr, value)       do { if (value) __set_bit(nr, addr); else __clear_bit(nr, addr); } while (0)

/* --- searching --------------------------------------------------------- */

unsigned long _find_next_bit(const unsigned long *addr, unsigned long nbits, unsigned long start, unsigned long invert);
unsigned long _find_first_bit(const unsigned long *addr, unsigned long size);
unsigned long _find_first_zero_bit(const unsigned long *addr, unsigned long size);
unsigned long _find_last_bit(const unsigned long *addr, unsigned long size);
unsigned long _find_next_and_bit(const unsigned long *addr1, const unsigned long *addr2, unsigned long nbits, unsigned long start);

#define find_next_bit(addr, size, off)      _find_next_bit(addr, size, off, 0UL)
#define find_next_zero_bit(addr, size, off) _find_next_bit(addr, size, off, ~0UL)
#define find_first_bit(addr, size)          _find_first_bit(addr, size)
#define find_first_zero_bit(addr, size)     _find_first_zero_bit(addr, size)
#define find_last_bit(addr, size)           _find_last_bit(addr, size)
#define find_next_and_bit(a1, a2, size, off) _find_next_and_bit(a1, a2, size, off)

#define for_each_set_bit(bit, addr, size) \
    for ((bit) = find_first_bit((addr), (size)); (bit) < (size); (bit) = find_next_bit((addr), (size), (bit) + 1))
#define for_each_set_bit_from(bit, addr, size) \
    for ((bit) = find_next_bit((addr), (size), (bit)); (bit) < (size); (bit) = find_next_bit((addr), (size), (bit) + 1))
#define for_each_clear_bit(bit, addr, size) \
    for ((bit) = find_first_zero_bit((addr), (size)); (bit) < (size); (bit) = find_next_zero_bit((addr), (size), (bit) + 1))
#define for_each_clear_bit_from(bit, addr, size) \
    for ((bit) = find_next_zero_bit((addr), (size), (bit)); (bit) < (size); (bit) = find_next_zero_bit((addr), (size), (bit) + 1))
#define for_each_set_bit_wrap(bit, addr, size, start) \
    for ((bit) = find_first_bit((addr), (size)); (bit) < (size); (bit) = find_next_bit((addr), (size), (bit) + 1))

/* iterate over set bits of a plain word (bitmask), lowest first */
#define for_each_set_bit_in_word(bit, word) \
    for (unsigned long __w = (word); __w && (((bit) = __ffs(__w)), 1); __w &= __w - 1)

#endif /* _LINUX_BITOPS_H_ */
