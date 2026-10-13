/*
    Copyright (C) 1995-2001, The AROS Development Team. All rights reserved.

    Desc:
*/



#include "nvdisk_intern.h"
#include <nvdisk_arch.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <libraries/nonvolatile.h>


/*****************************************************************************

    NAME */

        AROS_LH3(BOOL, SetNVDProtection,

/*  SYNOPSIS */

        AROS_LHA(STRPTR, appName,  A0),
        AROS_LHA(STRPTR, itemName, A1),
        AROS_LHA(LONG,   mask,     D0),

/*  LOCATION */

        struct Library *, nvdBase, 9, NVDisk)

/*  FUNCTION

    Set the filesystem protection flags of a nonvolatile data item.

    INPUTS

    appName   --  name of the application owning the item
    itemName  --  name of the item whose protection is to be changed
    mask      --  protection flags passed to SetProtection()

    RESULT

    TRUE if the filesystem-backed SetProtection() call succeeds;
    FALSE if the application directory cannot be locked or the
    protection operation fails.

    NOTES

    The filesystem-backed implementation locks the application
    directory but does not make it the current directory before
    calling SetProtection(itemName, mask). Consequently, a successful
    call is not proof that the intended application's item was changed.
    An active native backend delegates to NVDisk_ArchProtect().

    Callers should use nonvolatile.library/SetNVProtection().

    SEE ALSO

    nonvolatile.library/SetNVProtection(), <libraries/nonvolatile.h>

    HISTORY

    April 2000, SDuvan -- implemented

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    if (NVDisk_ArchActive(GPB(nvdBase)))
        return NVDisk_ArchProtect(GPB(nvdBase), appName, itemName, mask);

    BPTR lock;
    BOOL result = FALSE;
    BPTR oldCDir = CurrentDir(GPB(nvdBase)->nvd_location);

    if((lock = Lock(appName, SHARED_LOCK)) != BNULL)
    {
        result = SetProtection(itemName, mask) == DOSTRUE ? TRUE : FALSE;

        UnLock(lock);
    }

    CurrentDir(oldCDir);

    return result;

    AROS_LIBFUNC_EXIT
} /* SetProtection */
