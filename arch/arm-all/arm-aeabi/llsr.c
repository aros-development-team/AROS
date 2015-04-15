/*
    Copyright (C) 2009-2015, The AROS Development Team. All rights reserved.
    $Id$
 */

#include <inttypes.h>

uint64_t __aeabi_llsr(uint64_t val, int num)
{
    union {
        uint64_t v64;
        struct {
            uint32_t lo;
            uint32_t hi;
        } v32;
    } u;

    if (num == 0)
        return val;

    if (num > 63)
        return 0;

    u.v64 = val;

    {
        if (num > 31)
        {
            u.v32.lo = u.v32.hi >> (num - 32);
            u.v32.hi = 0;
        }
        else
        {
            uint32_t reminder = u.v32.hi << (32-num);
            u.v32.lo = reminder | (u.v32.lo >> num);
            u.v32.hi >>= num;
        }
    }

    return u.v64;
}
