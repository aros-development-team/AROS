/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#include "nvdisk_intern.h"
#include <dos/dos.h>
#include <proto/dos.h>
#include <libraries/nonvolatile.h>


AROS_LH4(LONG, WriteNVDData,

/*  SYNOPSIS */

	AROS_LHA(STRPTR, appName,  A0),
	AROS_LHA(STRPTR, itemName, A1),
	AROS_LHA(APTR,   data,     A2),
	AROS_LHA(LONG,   length,   A3),

/*  LOCATION */

	struct Library *, nvdBase, 6, NVDisk)

/*  FUNCTION

    Write data to the nonvolatile memory.

    INPUTS

    appName   --  the name of the application owning the data
    itemName  --  the name of the data to write
    data      --  the data to write
    length    --  the size of the data in bytes

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    nonvolatile.library/StoreNV()

    INTERNALS

    HISTORY

    November 2000,  SDuvan  --  implemeneted

******************************************************************************/

{
    AROS_LIBFUNC_INIT    

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

