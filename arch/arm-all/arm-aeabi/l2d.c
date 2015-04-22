/*
    Copyright (C) 2009-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <inttypes.h>

uint64_t __aeabi_ul2d(uint64_t val)
{
    uint64_t exp = 0;
    uint64_t result = 0;

    if (val == 0)
        return 0;

    exp = __builtin_clzl(val);

    if (exp <= 9)
        val >>= (10 - exp);
    else
        val <<= (exp - 10);

    if ((val & 1))
    {
        val ++;
        if (0x40000000000000ULL == val) exp++;
    }

    val >>= 1;
    val &= 0xfffffffffffffULL;
    exp = 1022 + 64 - exp;

    exp <<= 52;
    result = val | exp;

    return result;
}

uint64_t __aeabi_l2d(int64_t val)
{
    if (val == 0)
    return 0;

    if (val < 0)
        return (uint64_t)0x8000000000000000 | __aeabi_ul2d((uint64_t)-val);
    else
        return __aeabi_ul2d((uint64_t)val);
}
