/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#include <aros/debug.h>

#include "nvdisk_intern.h"

#include <dos/dos.h>
#include <proto/dos.h>
#include <libraries/nonvolatile.h>


AROS_LH2(BOOL, DeleteData,

/*  SYNOPSIS */

	AROS_LHA(STRPTR, appName,  A0),
	AROS_LHA(STRPTR, itemName, A1),

/*  LOCATION */

	struct Library *, nvdBase, 7, NVDisk)

/*  FUNCTION

    Delete a piece of data in the nonvolatile memory.

    INPUTS

    appName   --  the application owning the data to be deleted
    itemName  --  name of the data to be deleted

    RESULT
    
    Success / failure indicator.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT    

    BPTR oldCDir = CurrentDir(GPB(nvdBase)->nvd_location);
    BPTR lock = Lock(appName, SHARED_LOCK);
    BOOL retval;

    //    BPTR file = Open("T:temp", MODE_NEWFILE);
    //    Close(file);

    D(bug("Entering DeleteData()"));

    if(lock == NULL)
    {
	//	file = Open("T:temp2", MODE_NEWFILE);
	//	Close(file);

	D(bug("Could not lock directory %s", appName));
	CurrentDir(oldCDir);
	return FALSE;
    }

    D(bug("Deleting file %s", itemName));

    //    file = Open("T:temp3", MODE_NEWFILE);
    //    Close(file);

    CurrentDir(lock);

    //    file = Open("T:temp4", MODE_NEWFILE);
    //    Close(file);

    retval = DeleteFile(itemName) == DOSTRUE ? TRUE : FALSE;

    UnLock(lock);
    CurrentDir(oldCDir);

    return retval;

    AROS_LIBFUNC_EXIT
} /* DeleteData */
