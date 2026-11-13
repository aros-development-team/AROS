/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: WriteBattMem() function.
*/
#include <proto/battclock.h>

#include "battmem_intern.h"

/*****************************************************************************

    NAME */
#include <proto/battmem.h>

        AROS_LH3(ULONG, WriteBattMem,

/*  SYNOPSIS */
        AROS_LHA(CONST_APTR, buffer, A0),
        AROS_LHA(ULONG,      offset, D0),
        AROS_LHA(ULONG,      length, D1),

/*  LOCATION */
        struct BattMemBase *, BattMemBase, 4, Battmem)

/*  FUNCTION
        Copy a field from a buffer into the battery backed up memory.

    INPUTS
        buffer - the field to store, (length + 7) / 8 bytes.
        offset - bit offset of the field within the battery memory.
        length - size of the field in BITS, not bytes.

    RESULT
        Always zero.

    NOTES
        Writes that run past the end of the battery memory are truncated
        rather than failing. See <resources/battmembitsamiga.h> and
        <resources/battmembitsshared.h> for the assigned fields.

        A trailing partial byte is taken right aligned from the last byte
        of the buffer.

    EXAMPLE

    BUGS

    SEE ALSO
        ReadBattMem(), ObtainBattSemaphore()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Library *BattClockBase = BattMemBase->bm_BattClockBase;
    const UBYTE *buf = (const UBYTE *)buffer;
    ULONG bytes = length >> 3;
    ULONG bits = length & 7;

    while (bytes-- > 0)
    {
        WriteBattClockMem(*buf++, offset, 8);
        offset += 8;
    }

    if (bits)
        WriteBattClockMem(*buf, offset, bits);

    return 0;

    AROS_LIBFUNC_EXIT
} /* WriteBattMem */
