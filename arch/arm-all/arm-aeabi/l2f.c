/*
    Copyright (C) 2009-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <inttypes.h>

uint32_t __aeabi_ul2f(uint64_t val)
{
    int exp = 0;
    uint32_t result = 0;
    uint32_t v = 0;

    if (val == 0)
        return 0;

    exp = 64 - __builtin_clzl(val);

    if (exp >= 26)
        v = (uint32_t)(val >> (exp - 25));
    else
        v = (uint32_t)(val << (25 - exp));

    if ((v & 1))
    {
        v ++;
        if (0x02000000 == v) exp++;
    }

    result = (v >> 1) & 0x7fffff;

    exp += 0x7E;

    /* adapt Exponent to IEEESP-Format */
    exp <<= 23;
    result |= exp;

    return result;
}

uint32_t __aeabi_l2f(int64_t val)
{
    if (val == 0)
        return 0;
    else if (val < 0)
        return 0x80000000 | __aeabi_ul2f((uint64_t)(-val));
    else
        return __aeabi_ul2f(val);
}
