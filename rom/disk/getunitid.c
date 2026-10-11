/*
    Copyright (C) 2010, The AROS Development Team. All rights reserved.

    Desc: GetUnitID() function.
*/

#include <proto/disk.h>
#include <resources/disk.h>

/*****************************************************************************

    NAME */
        AROS_LH1(ULONG, GetUnitID,

/*  SYNOPSIS */
         AROS_LHA(LONG, unitNum, D0),

/*  LOCATION */
         struct DiscResource *, DiskBase, 5, Disk)
/*  FUNCTION
        Return the cached hardware identification value for a floppy
        drive. This function does not probe the drive or test for a disk.

    INPUTS
        unitNum - Drive number from 0 through 3 inclusive.

    RESULT
        The cached ULONG identification value. Defined values in
        <resources/disk.h> include DRT_AMIGA, DRT_37422D2S, DRT_EMPTY and
        DRT_150RPM. Other bit patterns are returned unchanged.

    NOTES
        The native Amiga resource fills the cache during initialization;
        ReadUnitID() refreshes it. The unit number is not checked. There
        is no separate error result, allocation or wait in this call.

    SEE ALSO
        ReadUnitID()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

        return DiskBase->dr_UnitID[unitNum];

    AROS_LIBFUNC_EXIT
}
