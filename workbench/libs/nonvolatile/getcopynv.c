/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#include <libraries/nonvolatile.h>
#include <proto/exec.h>
#include <proto/nvdisk.h>
#include "nonvolatile_intern.h"

AROS_LH3(APTR, GetCopyNV,

/*  SYNOPSIS */

	AROS_LHA(STRPTR, appName,        A0),
	AROS_LHA(STRPTR, itemName,       A1),
	AROS_LHA(BOOL,   killRequesters, D1),

/*  LOCATION */

	struct Library *, nvBase, 5, Nonvolatile)

/*  FUNCTION

    Search the nonvolatile memory for the object 'itemName' allocated by
    'appName' and return a copy of the item.

    INPUTS

    appName         --  name of the application that has allocated the item
    itemName        --  the object to look for
    killRequesters  --  if TRUE no system requesters will be displayed during
                        the operation of this function

    RESULT

    Pointer to the data assocated with 'itemName' as allocated by 'appName'.
    
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    FreeNVData(), <libraries/nonvolatile.h>

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    struct Process *me = (struct Process *)FindTask(NULL);
    APTR oldReq = me->pr_WindowPtr;
    APTR result;

    if(appName == NULL || itemName == NULL)
	return NULL;

    if(strpbrk(appName, ":/") != NULL ||
       strpbrk(itemName, ":/") != NULL)
	return NULL;

    if(killRequesters)
	me->pr_WindowPtr = (APTR)-1;

    result = ReadData(appName, itemName);

    if(killRequesters)
	me->pr_WindowPtr = oldReq;

    return result;

    AROS_LIBFUNC_EXIT
} /* GetCopyNV */
