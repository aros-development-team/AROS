/*
    Copyright (C) 2010, The AROS Development Team. All rights reserved.

    Desc: ReadUnitID() function.
*/

#include <proto/disk.h>
#include <resources/disk.h>

extern void readunitid_internal (struct DiscResource*, LONG unitNum);

/*****************************************************************************

    NAME */
        AROS_LH1(ULONG, ReadUnitID,

/*  SYNOPSIS */
         AROS_LHA(LONG, unitNum, D0),

/*  LOCATION */
         struct DiscResource *, DiskBase, 6, Disk)
/*  FUNCTION
        Invoke the architecture-specific drive identification routine and
        return the cached identification value for the selected drive.
        On native Amiga hardware this reads the 32-bit drive ID stream
        through the CIA motor and drive-select lines and updates the cache.

    INPUTS
        unitNum - Drive number from 0 through 3 inclusive.

    RESULT
        The resulting cached ULONG ID. See the DRT_* constants in
        <resources/disk.h>. This is a drive identification value, not a
        media-change or disk-presence result. There is no separate error
        status.

    NOTES
        The native Amiga implementation changes motor/select control
        lines and does not acquire shared disk hardware ownership itself.
        Obtain ownership with GetUnit() and coordinate with other users
        before probing; release ownership with GiveUnit() afterwards.
        No unit-range check or wait is performed by this function.

        On native Amiga, an all-ones response for unit 0 is treated as
        DRT_AMIGA on machines without the CD32 Akiko identification. On
        CD32 it remains DRT_EMPTY. Other ID bit patterns are preserved.

        The generic architecture backend does not implement probing or
        resource initialization; availability requires a working backend.

    SEE ALSO
        GetUnitID(), GetUnit(), GiveUnit()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

        readunitid_internal (DiskBase, unitNum);
        return DiskBase->dr_UnitID[unitNum];

    AROS_LIBFUNC_EXIT
}
