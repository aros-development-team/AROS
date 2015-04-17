/*
    Copyright (C) 2009-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <inttypes.h>

uint64_t __aeabi_ui2d(uint32_t val)
{
    int exp = 0;
    uint64_t result = 0;

    if (val == 0)
        return 0;

    exp = 32 - __builtin_clz(val);

    result = ((uint64_t)val) << (53 - exp);
    result &= (uint64_t)0x000fffffffffffff;

    exp += 0x3fe;

    result |= (uint64_t)exp << 52;

    return result;
}

uint64_t __aeabi_i2d(int32_t val)
{
    if (val == 0)
        return 0;

    if (val < 0)
        return (uint64_t)0x8000000000000000 | __aeabi_ui2d((uint32_t)-val);
    else
        return __aeabi_ui2d((uint32_t)val);
}
