/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: ReadBattMem() function.
*/
#include <proto/battclock.h>

#include "battmem_intern.h"

/*****************************************************************************

    NAME */
#include <proto/battmem.h>

        AROS_LH3(ULONG, ReadBattMem,

/*  SYNOPSIS */
        AROS_LHA(APTR,  buffer, A0),
        AROS_LHA(ULONG, offset, D0),
        AROS_LHA(ULONG, length, D1),

/*  LOCATION */
        struct BattMemBase *, BattMemBase, 3, Battmem)

/*  FUNCTION
        Copy a field out of the battery backed up memory into a buffer.

    INPUTS
        buffer - where to put the field. It has to be large enough to hold
                 (length + 7) / 8 bytes.
        offset - bit offset of the field within the battery memory.
        length - size of the field in BITS, not bytes.

    RESULT
        Always zero.

    NOTES
        Bit offsets and lengths beyond the end of the battery memory read
        back as zero rather than failing. See <resources/battmembitsamiga.h>
        and <resources/battmembitsshared.h> for the assigned fields.

        A trailing partial byte is right aligned in the last byte written.

    EXAMPLE

    BUGS

    SEE ALSO
        WriteBattMem(), ObtainBattSemaphore()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Library *BattClockBase = BattMemBase->bm_BattClockBase;
    UBYTE *buf = (UBYTE *)buffer;
    ULONG bytes = length >> 3;
    ULONG bits = length & 7;

    while (bytes-- > 0)
    {
        *buf++ = (UBYTE)ReadBattClockMem(offset, 8);
        offset += 8;
    }

    if (bits)
        *buf = (UBYTE)ReadBattClockMem(offset, bits);

    return 0;

    AROS_LIBFUNC_EXIT
} /* ReadBattMem */
