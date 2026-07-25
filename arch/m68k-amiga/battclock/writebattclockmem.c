/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: WriteBattClockMem() function.
*/
#define DEBUG 0

#include <aros/debug.h>
#include <aros/libcall.h>

#include <proto/exec.h>

#include "battclock_intern.h"

#include <proto/battclock.h>

/* See rom/battclock/writebattclockmem.c for documentation */

AROS_LH3(void, WriteBattClockMem,
    AROS_LHA(ULONG, data,   D0),
    AROS_LHA(ULONG, offset, D1),
    AROS_LHA(ULONG, length, D2),
    struct BattClockBase *, BattClockBase, 5, Battclock)
{
    AROS_LIBFUNC_INIT

    UBYTE buf[BATTMEM_REGS];
    UBYTE mask;
    ULONG i, bit;

    D(bug("WriteBattClockMem(%08x, %d, %d)\n", data, offset, length));

    if (length > 32)
        length = 32;
    if (length == 0 || offset >= BATTMEM_BITS)
        return;
    if (offset + length > BATTMEM_BITS)
    {
        /* Drop the bits that would not fit rather than wrapping around. */
        data >>= offset + length - BATTMEM_BITS;
        length = BATTMEM_BITS - offset;
    }

    Disable();
    if (battmemload(BattClockBase, buf))
    {
        /* Bit 0 is the most significant bit of the first byte. */
        for (i = 0; i < length; i++)
        {
            bit = offset + i;
            mask = 0x80 >> (bit & 7);
            if (data & (1UL << (length - 1 - i)))
                buf[bit >> 3] |= mask;
            else
                buf[bit >> 3] &= ~mask;
        }
        battmemstore(BattClockBase, buf);
    }
    Enable();

    AROS_LIBFUNC_EXIT

} /* WriteBattClockMem */
