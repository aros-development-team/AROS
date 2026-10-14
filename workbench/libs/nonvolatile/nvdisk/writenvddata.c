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

        AROS_LH4(LONG, WriteNVDData,

/*  SYNOPSIS */

        AROS_LHA(STRPTR, appName,  A0),
        AROS_LHA(STRPTR, itemName, A1),
        AROS_LHA(APTR,   data,     A2),
        AROS_LHA(LONG,   length,   A3),

/*  LOCATION */

        struct Library *, nvdBase, 6, NVDisk)

/*  FUNCTION

    Store an application's nonvolatile data item.

    INPUTS

    appName   --  name of the application owning the item
    itemName  --  name of the item to write
    data      --  pointer to the bytes to store
    length    --  number of bytes to write

    RESULT

    Zero if the filesystem-backed implementation does not detect an
    error. NVERR_WRITEPROT indicates a write-protected storage volume
    when application-directory creation fails. NVERR_FAIL indicates
    other directory-creation or file-open failures. NVERR_FATAL
    indicates a Write() result of -1.

    NOTES

    The filesystem-backed implementation creates the application
    directory if necessary and opens the item with MODE_NEWFILE.
    A positive short write is not treated as an error by this
    implementation. An active native backend returns the status from
    NVDisk_ArchWrite(), which may have different failure conditions.

    The public StoreNV() interface provides the application-facing
    operation and performs its own validation.

    SEE ALSO

    nonvolatile.library/StoreNV(), <libraries/nonvolatile.h>

    HISTORY

    November 2000, SDuvan -- implemented

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    if (NVDisk_ArchActive(GPB(nvdBase)))
        return NVDisk_ArchWrite(GPB(nvdBase), appName, itemName, data,
            length);

    LONG retval  = 0;
    BPTR oldCDir = CurrentDir(GPB(nvdBase)->nvd_location);
    BPTR lock    = Lock(appName, SHARED_LOCK);
    BPTR file;

    if(lock == BNULL)
    {
        lock = CreateDir(appName);

        if(lock == BNULL)
        {
            LONG err = IoErr();
            if(err == ERROR_WRITE_PROTECTED ||
               err == ERROR_DISK_WRITE_PROTECTED)
                return NVERR_WRITEPROT;
            else
                return NVERR_FAIL;
        }
    }

    /* 'lock' is now containing a valid lock on the 'appName' directory. */

    CurrentDir(lock);

    file = Open(itemName, MODE_NEWFILE);

    if(file == BNULL)
    {
        UnLock(lock);
        CurrentDir(oldCDir);
        return NVERR_FAIL;
    }

    if(Write(file, data, length) == -1)
    {
        retval = NVERR_FATAL;
    }

    Close(file);
    UnLock(lock);
    CurrentDir(oldCDir);

    return retval;

    AROS_LIBFUNC_EXIT
} /* WriteData */
