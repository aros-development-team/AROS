/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: ReadBattClockMem() function.
*/
#include "battclock_intern.h"

/*****************************************************************************

    NAME */
#include <proto/battclock.h>

        AROS_LH2(ULONG, ReadBattClockMem,

/*  SYNOPSIS */
        AROS_LHA(ULONG, offset, D1),
        AROS_LHA(ULONG, length, D2),

/*  LOCATION */
        APTR, BattClockBase, 4, Battclock)

/*  FUNCTION
        Read a field of up to 32 bits out of the battery backed up memory
        of the real time clock. This is the storage battmem.resource is
        built on; normal applications should use that resource instead.

    INPUTS
        offset - bit offset of the field, counted from the first bit of
                 the battery memory.
        length - size of the field in bits. Values above 32 are clamped
                 to 32.

    RESULT
        The field, right aligned. Zero if the clock has no battery memory,
        or if offset lies beyond the end of it.

    NOTES
        Not all clock chips have battery backed up memory. The MSM6242B
        fitted to some machines has none, and this function returns 0
        there.

    EXAMPLE

    BUGS

    SEE ALSO
        WriteBattClockMem()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

#warning battclock.resource functionality not added
    return 0;

    AROS_LIBFUNC_EXIT
} /* ReadBattClockMem */
