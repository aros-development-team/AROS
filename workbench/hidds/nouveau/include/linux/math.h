/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_MATH_H_
#define _LINUX_MATH_H_

#include <stdlib.h>
#include <linux/types.h>
#include <linux/compiler.h>
#include <linux/const.h>

#define DIV_ROUND_UP(n, d)          (((n) + (d) - 1) / (d))
#define __KERNEL_DIV_ROUND_UP(n, d) DIV_ROUND_UP(n, d)
#define DIV_ROUND_UP_ULL(ll, d)     ({ unsigned long long _tmp = (ll) + (d) - 1; do_div_u64_by_u32(&_tmp, (d)); _tmp; })
#define DIV_ROUND_DOWN_ULL(ll, d)   ({ unsigned long long _tmp = (ll); do_div_u64_by_u32(&_tmp, (d)); _tmp; })
#define DIV_ROUND_UP_SECTOR_T(ll, d) DIV_ROUND_UP_ULL(ll, d)
#define DIV_ROUND_CLOSEST(x, divisor) ({                                \
    typeof(x) __x = x; typeof(divisor) __d = divisor;                   \
    (((typeof(x))-1) > 0 || ((typeof(divisor))-1) > 0 || (((__x) > 0) == ((__d) > 0))) ? \
        (((__x) + ((__d) / 2)) / (__d)) : (((__x) - ((__d) / 2)) / (__d)); })
#define DIV_ROUND_CLOSEST_ULL(x, divisor) ({                            \
    typeof(divisor) __d = divisor;                                      \
    unsigned long long _tmp = (x) + (__d) / 2;                          \
    do_div_u64_by_u32(&_tmp, __d); _tmp; })
#undef roundup
#undef rounddown
#define roundup(x, y)   ({ typeof(y) __y = y; (((x) + (__y - 1)) / __y) * __y; })
#define rounddown(x, y) ({ typeof(x) __x = (x); __x - (__x % (y)); })
#define mult_frac(x, n, d) ({ typeof(x) q = (x) / (d); typeof(x) r = (x) % (d); q * (n) + r * (n) / (d); })
#define abs(x)          ({ typeof(x) __x = (x); __x < 0 ? -__x : __x; })
#define abs_diff(a, b)  ({ typeof(a) __a = (a); typeof(b) __b = (b); __a > __b ? (__a - __b) : (__b - __a); })
#define sector_div(a, b) do_div(a, b)

static inline u32 do_div_u64_by_u32(unsigned long long *n, u32 base)
{
    u32 rem = (u32)(*n % base);
    *n = *n / base;
    return rem;
}
#define do_div(n, base) ({ u32 __base = (base); u32 __rem; __rem = ((u64)(n)) % __base; (n) = ((u64)(n)) / __base; __rem; })

static inline u32 reciprocal_scale(u32 val, u32 ep_ro) { return (u32)(((u64)val * ep_ro) >> 32); }
static inline unsigned long gcd(unsigned long a, unsigned long b)
{
    while (b) { unsigned long t = a % b; a = b; b = t; }
    return a;
}
static inline u64 int_pow(u64 base, unsigned int exp)
{
    u64 result = 1;
    while (exp) { if (exp & 1) result *= base; exp >>= 1; base *= base; }
    return result;
}
unsigned long int_sqrt(unsigned long);

#endif /* _LINUX_MATH_H_ */
