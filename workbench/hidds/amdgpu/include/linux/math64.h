/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_MATH64_H_
#define _LINUX_MATH64_H_

#include <linux/types.h>
#include <linux/math.h>
#include <linux/types.h>
#include <linux/asm.h>

static inline u64 div_u64_rem(u64 dividend, u32 divisor, u32 *remainder)
{
    *remainder = (u32)(dividend % divisor);
    return dividend / divisor;
}
static inline s64 div_s64_rem(s64 dividend, s32 divisor, s32 *remainder)
{
    *remainder = (s32)(dividend % divisor);
    return dividend / divisor;
}
static inline u64 div64_u64_rem(u64 dividend, u64 divisor, u64 *remainder)
{
    *remainder = dividend % divisor;
    return dividend / divisor;
}
static inline u64 div64_u64(u64 dividend, u64 divisor) { return dividend / divisor; }
static inline s64 div64_s64(s64 dividend, s64 divisor) { return dividend / divisor; }
static inline u64 div_u64(u64 dividend, u32 divisor)   { return dividend / divisor; }
static inline s64 div_s64(s64 dividend, s32 divisor)   { return dividend / divisor; }
static inline u64 mul_u32_u32(u32 a, u32 b)            { return (u64)a * b; }
#if defined(__SIZEOF_INT128__)
static inline u64 mul_u64_u32_shr(u64 a, u32 mul, unsigned int shift)
{
    return (u64)(((unsigned __int128)a * mul) >> shift);
}
static inline u64 mul_u64_u64_shr(u64 a, u64 mul, unsigned int shift)
{
    return (u64)(((unsigned __int128)a * mul) >> shift);
}
static inline u64 mul_u64_u32_div(u64 a, u32 mul, u32 divisor)
{
    return (u64)(((unsigned __int128)a * mul) / divisor);
}
static inline u64 mul_u64_u64_div_u64(u64 a, u64 mul, u64 div)
{
    return (u64)(((unsigned __int128)a * mul) / div);
}
static inline u64 mul_u64_u32_div_u64(u64 a, u32 mul, u64 div)
{
    return (u64)(((unsigned __int128)a * mul) / div);
}
static inline u64 mul_u64_add_u64_div_u64(u64 a, u64 mul, u64 add, u64 div)
{
    return (u64)(((unsigned __int128)a * mul + add) / div);
}
#else
/* 32-bit targets: split multiplications, good enough for the clock maths */
static inline u64 mul_u64_u32_shr(u64 a, u32 mul, unsigned int shift)
{
    u32 ah = (u32)(a >> 32), al = (u32)a;
    u64 ret = mul_u32_u32(al, mul) >> shift;
    if (ah)
        ret += mul_u32_u32(ah, mul) << (32 - shift);
    return ret;
}
static inline u64 mul_u64_u64_shr(u64 a, u64 mul, unsigned int shift)
{
    return mul_u64_u32_shr(a, (u32)mul, shift);
}
u64 mul_u64_u64_div_u64(u64 a, u64 mul, u64 div);
static inline u64 mul_u64_u32_div(u64 a, u32 mul, u32 divisor)
{
    return mul_u64_u64_div_u64(a, mul, divisor);
}
static inline u64 mul_u64_u32_div_u64(u64 a, u32 mul, u64 div)
{
    return mul_u64_u64_div_u64(a, mul, div);
}
static inline u64 mul_u64_add_u64_div_u64(u64 a, u64 mul, u64 add, u64 div)
{
    return mul_u64_u64_div_u64(a, mul, div) + add / div;
}
#endif
static inline u64 iter_div_u64_rem(u64 dividend, u32 divisor, u64 *remainder)
{
    u32 r;
    u64 q = div_u64_rem(dividend, divisor, &r);
    *remainder = r;
    return q;
}
#define DIV64_U64_ROUND_UP(ll, d)       ({ u64 _tmp = (d); div64_u64((ll) + _tmp - 1, _tmp); })
#define DIV64_U64_ROUND_CLOSEST(ll, d)  ({ u64 _tmp = (d); div64_u64((ll) + _tmp / 2, _tmp); })
#define DIV_U64_ROUND_CLOSEST(ll, d)    ({ u32 _tmp = (d); div_u64((ll) + _tmp / 2, _tmp); })
#define DIV_S64_ROUND_CLOSEST(ll, d)    ({ s64 __ll = (ll); s32 __d = (d); ((__ll > 0) == (__d > 0)) ? div_s64(__ll + __d / 2, __d) : div_s64(__ll - __d / 2, __d); })

#endif /* _LINUX_MATH64_H_ */
