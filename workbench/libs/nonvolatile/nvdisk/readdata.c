/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#include "nvdisk_intern.h"
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <exec/memory.h>


AROS_LH2(APTR, ReadData,

/*  SYNOPSIS */

	AROS_LHA(STRPTR, appName,  A0),
	AROS_LHA(STRPTR, itemName, A1),

/*  LOCATION */

	struct Library *, nvdBase, 5, NVDisk)

/*  FUNCTION

    Read the nv data corresponding to appname, itemname.

    INPUTS

    appname   --  the name of the application owning the data
    itemname  --  the name of the data to retrieve

    RESULT

    A pointer to the data read. If something went wrong NULL is returned.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT    

    APTR  mem;
    BPTR  lock, lock2;
    BPTR  oldCDir;

    /* Allocate a FileInfoBlock to be able to determine the size of the
       file containing the data. */
    struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);

    if(fib == NULL)
	return NULL;

    oldCDir = CurrentDir(GPB(nvdBase)->nvd_location);
    lock = Lock(appName, SHARED_LOCK);

    if(lock == NULL)
    {
	CurrentDir(oldCDir);
	return NULL;
    }

    CurrentDir(lock);

    lock2 = Lock(itemName, SHARED_LOCK);

    // We have now found the file -- check how big it is
    Examine(lock2, fib);

    mem = AllocVec(fib->fib_Size, MEMF_ANY);

    Read(lock2, mem, fib->fib_Size);

    // Release resources and restore the old state
    FreeDosObject(DOS_FIB, fib);
    UnLock(lock2);
    UnLock(lock);
    CurrentDir(oldCDir);

    return mem;

    AROS_LIBFUNC_EXIT
} /* ReadData */
