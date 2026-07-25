/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: WriteBattClockMem() function.
*/
#include "battclock_intern.h"

/*****************************************************************************

    NAME */
#include <proto/battclock.h>

        AROS_LH3(void, WriteBattClockMem,

/*  SYNOPSIS */
        AROS_LHA(ULONG, data,   D0),
        AROS_LHA(ULONG, offset, D1),
        AROS_LHA(ULONG, length, D2),

/*  LOCATION */
        APTR, BattClockBase, 5, Battclock)

/*  FUNCTION
        Write a field of up to 32 bits into the battery backed up memory
        of the real time clock. This is the storage battmem.resource is
        built on; normal applications should use that resource instead.

    INPUTS
        data   - the field value, right aligned.
        offset - bit offset of the field, counted from the first bit of
                 the battery memory.
        length - size of the field in bits. Values above 32 are clamped
                 to 32.

    RESULT

    NOTES
        Not all clock chips have battery backed up memory. The MSM6242B
        fitted to some machines has none, and this function does nothing
        there. Writes that start beyond the end of the battery memory are
        ignored; writes that run past the end are truncated.

    EXAMPLE

    BUGS

    SEE ALSO
        ReadBattClockMem()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

#warning battclock.resource functionality not added

    AROS_LIBFUNC_EXIT
} /* WriteBattClockMem */
