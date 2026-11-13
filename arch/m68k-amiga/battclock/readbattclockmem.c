/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: ReadBattClockMem() function.
*/
#define DEBUG 0

#include <aros/debug.h>
#include <aros/libcall.h>

#include <proto/exec.h>

#include "battclock_intern.h"

#include <proto/battclock.h>

/* See rom/battclock/readbattclockmem.c for documentation */

AROS_LH2(ULONG, ReadBattClockMem,
    AROS_LHA(ULONG, offset, D1),
    AROS_LHA(ULONG, length, D2),
    struct BattClockBase *, BattClockBase, 4, Battclock)
{
    AROS_LIBFUNC_INIT

    UBYTE buf[BATTMEM_REGS];
    ULONG value = 0;
    ULONG i, bit;

    D(bug("ReadBattClockMem(%d, %d)\n", offset, length));

    if (length > 32)
        length = 32;
    if (length == 0 || offset >= BATTMEM_BITS)
        return 0;

    Disable();
    if (!battmemload(BattClockBase, buf))
    {
        Enable();
        return 0;
    }
    Enable();

    /* Bit 0 is the most significant bit of the first byte. */
    for (i = 0; i < length; i++)
    {
        bit = offset + i;
        value <<= 1;
        if (bit < BATTMEM_BITS && (buf[bit >> 3] & (0x80 >> (bit & 7))))
            value |= 1;
    }

    D(bug("ReadBattClockMem = %08x\n", value));
    return value;

    AROS_LIBFUNC_EXIT

} /* ReadBattClockMem */
