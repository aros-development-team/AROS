/*
    Copyright (C) 2010, The AROS Development Team. All rights reserved.

    Desc: FreeUnit() function.
*/

#include <proto/disk.h>
#include <resources/disk.h>

/*****************************************************************************

    NAME */
        AROS_LH1(void, FreeUnit,

/*  SYNOPSIS */
         AROS_LHA(LONG, unitNum, D0),

/*  LOCATION */
         struct DiscResource *, DiskBase, 2, Disk)
/*  FUNCTION
        Clear the allocation bit for a floppy drive unit previously
        reserved with AllocUnit().

    INPUTS
        unitNum - Drive number from 0 through 3 inclusive, previously
                  reserved by the caller.

    RESULT
        None.

    NOTES
        This does not release shared hardware ownership, stop disk I/O,
        remove an interrupt descriptor or wake a GetUnit() waiter. Finish
        using the unit and separately release any shared hardware ownership
        with GiveUnit().

        Neither the unit number nor ownership is checked. The bitmap
        update has no internal critical section; callers must serialize
        competing allocation and release operations.

    SEE ALSO
        AllocUnit(), GetUnit(), GiveUnit()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

        DiskBase->dr_Flags &= ~(1 << unitNum);

    AROS_LIBFUNC_EXIT
}
