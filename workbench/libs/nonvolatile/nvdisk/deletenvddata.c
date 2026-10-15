/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/debug.h>

#include "nvdisk_intern.h"
#include <nvdisk_arch.h>

#include <dos/dos.h>
#include <proto/dos.h>
#include <libraries/nonvolatile.h>

//static int cxbug; /* For cxref bug */

/*****************************************************************************

    NAME */
        AROS_LH2(BOOL, DeleteNVDData,

/*  SYNOPSIS */

        AROS_LHA(STRPTR, appName,  A0),
        AROS_LHA(STRPTR, itemName, A1),

/*  LOCATION */

        struct Library *, nvdBase, 7, NVDisk)

/*  FUNCTION
    Delete a stored item from an application's nonvolatile data area.
    In the filesystem-backed implementation, locate the application's
    directory beneath the configured storage root and delete the item file.

    INPUTS
        appName  - Name of the application directory relative to the
                   nonvolatile storage root.
        itemName - Name of the item file to delete from that directory.

    RESULT
        TRUE if the item was deleted; FALSE if the application directory
        cannot be locked or the file cannot be deleted.

    NOTES
        The filesystem-backed operation temporarily changes the current
        directory and restores it before returning. An active native
        storage backend, if present, handles deletion through its own
        NVDisk_ArchDelete() implementation.

        The public DeleteNV() entry point validates item names and supports
        suppression of system requesters. Callers should normally use it
        instead of invoking the disk-backed interface directly.

    SEE ALSO
        nonvolatile.library/DeleteNV()

    HISTORY
        November 2000, SDuvan -- implemented

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    if (NVDisk_ArchActive(GPB(nvdBase)))
        return NVDisk_ArchDelete(GPB(nvdBase), appName, itemName);

    BPTR oldCDir = CurrentDir(GPB(nvdBase)->nvd_location);
    BPTR lock = Lock(appName, SHARED_LOCK);
    BOOL retval;

    D(bug("Entering DeleteData()"));

    if(lock == BNULL)
    {
        D(bug("Could not lock directory %s", appName));
        CurrentDir(oldCDir);
        return FALSE;
    }

    D(bug("Deleting file %s", itemName));

    CurrentDir(lock);

    retval = DeleteFile(itemName) == DOSTRUE ? TRUE : FALSE;

    UnLock(lock);
    CurrentDir(oldCDir);

    return retval;

    AROS_LIBFUNC_EXIT
} /* DeleteData */
