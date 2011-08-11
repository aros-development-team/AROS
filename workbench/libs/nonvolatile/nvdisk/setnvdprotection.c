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


AROS_LH3(BOOL, SetNVDProtection,

/*  SYNOPSIS */

	AROS_LHA(STRPTR, appName,  A0),
	AROS_LHA(STRPTR, itemName, A1),
	AROS_LHA(LONG,   mask,     D0),

/*  LOCATION */

	struct Library *, nvdBase, 9, NVDisk)

/*  FUNCTION

    Protect an item saved in nonvolatile memory.

    INPUTS

    appName   --  the application owning the item
    itemName  --  the item to protect
    mask      --  protection status to inflict

    RESULT

    The memory item 'itemName' will be protected as specified by the 'mask'.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

    April 2000,  SDuvan  --  implemented
    
******************************************************************************/

{
    AROS_LIBFUNC_INIT    

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
