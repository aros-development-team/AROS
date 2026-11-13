/*
    Copyright (C) 1995-2001, The AROS Development Team. All rights reserved.

    Desc: AllocUnit() function.
*/

#include <proto/disk.h>
#include <resources/disk.h>

/*****************************************************************************

    NAME */
        AROS_LH1(BOOL, AllocUnit,

/*  SYNOPSIS */
         AROS_LHA(LONG, unitNum, D0),

/*  LOCATION */
         struct DiscResource *, DiskBase, 1, Disk)
/*  FUNCTION
        Reserve a floppy drive unit in disk.resource's allocation bitmap.
        This reservation is separate from ownership of the shared disk
        hardware, which is acquired with GetUnit().

    INPUTS
        unitNum - Drive number from 0 through 3 inclusive.

    RESULT
        TRUE if the unit was free and has now been reserved; FALSE if it
        was already reserved. No task ownership is recorded.

    NOTES
        Balance a successful reservation with FreeUnit(). The unit number
        is not checked. The bitmap update has no internal critical section;
        callers must serialize competing allocation and release operations.

    SEE ALSO
        FreeUnit(), GetUnit(), GiveUnit()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

        if (DiskBase->dr_Flags & (1 << unitNum))
                return 0;
        DiskBase->dr_Flags |= 1 << unitNum;
    return 1;

    AROS_LIBFUNC_EXIT
}
