/*
    Copyright (C) 2009-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <inttypes.h>

uint32_t __aeabi_ui2f(uint32_t val)
{
    int exp = 0;
    uint32_t result = 0;

    if (val == 0)
        return 0;

    exp = __builtin_clz(val);

    if (exp <= 6)
        val >>= (7 - exp);
    else
        val <<= (exp - 7);

    if ((val & 1))
    {
        val ++;
        if (0x02000000 == val) exp++;
    }

    val >>= 1;
    val &= 0x7fffff;

    exp = 0x7E + 32 - exp;

    /* adapt Exponent to IEEESP-Format */
    exp <<= 23;
    result = val | exp;

    return result;
}

uint32_t __aeabi_i2f(int32_t val)
{
    if (val == 0)
        return 0;

    if (val < 0)
        return 0x80000000 | __aeabi_ui2f((uint32_t)(-val));
    else
        return __aeabi_ui2f(val);
}

