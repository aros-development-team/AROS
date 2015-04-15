/*
    Copyright (C) 2009-2015, The AROS Development Team. All rights reserved.
    $Id$
 */

#include <inttypes.h>

int64_t __aeabi_lasr(int64_t val, int num)
{
    union {
        int64_t v64;
        struct {
            uint32_t lo;
            int32_t hi;
        } v32;
    } u;

    if (num == 0)
        return val;

    if (num > 63)
    {
        if (val < 0)
            return -1;
        else
            return 0;
    }

    u.v64 = val;

    if (num > 31)
    {
        u.v32.lo = (uint32_t)(u.v32.hi >> (num - 32));
        u.v32.hi >>= 31;
    }
    else
    {
        uint32_t reminder = ((uint32_t)u.v32.hi) << (32-num);
        u.v32.lo = reminder | (u.v32.lo >> num);
        u.v32.hi >>= num;
    }

    return u.v64;
}
