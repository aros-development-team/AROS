/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#define  DEBUG  1

#include <aros/debug.h>

#include <libraries/nonvolatile.h>
#include <dos/dosextens.h>
#include <proto/nvdisk.h>
#include "nonvolatile_intern.h"

#include <string.h>


AROS_LH3(BOOL, DeleteNV,

/*  SYNOPSIS */

	AROS_LHA(STRPTR, appName,  A0),
	AROS_LHA(STRPTR, itemName, A1),
	AROS_LHA(BOOL,   killRequesters, D0),

/*  LOCATION */

	struct Library *, nvBase, 8, Nonvolatile)

/*  FUNCTION

    Delete a piece of data in the nonvolatile memory.

    INPUTS

    appName         --  the application owning the data to be deleted; maximum
                        length 31
    itemName        --  name of the data to be deleted; maximum length 31
    killRequesters  --  if set to TRUE no system requesters will be displayed
                        during the deletion operation

    RESULT
    
    Success / failure indicator.

    NOTES

    The 'appName' and 'itemName' strings may NOT include the characters
    '/' or ':'.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    struct Process *me = (struct Process *)FindTask(NULL);
    APTR oldReq = me->pr_WindowPtr;
    BOOL result;

    D(bug("Entering DeleteNV()\n"));

    if(appName == NULL || itemName == NULL)
	return FALSE;

    if(strpbrk(appName, ":/") != NULL ||
       strpbrk(itemName, ":/") != NULL)
	return FALSE;

    if(killRequesters)
	me->pr_WindowPtr = (APTR)-1;

    D(bug("Calling DeleteData()\n"));

    result = DeleteData(appName, itemName);

    if(killRequesters)
	me->pr_WindowPtr = oldReq;

    return result;

    AROS_LIBFUNC_EXIT
} /* DeleteNV */
