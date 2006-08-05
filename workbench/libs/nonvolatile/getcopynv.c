/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>
#include <dos/dosextens.h>

/*****************************************************************************

    NAME */

#include <libraries/nonvolatile.h>
#include <proto/exec.h>
#include <proto/nvdisk.h>

AROS_LH3(APTR, GetCopyNV,

/*  SYNOPSIS */

	AROS_LHA(STRPTR, appName,        A0),
	AROS_LHA(STRPTR, itemName,       A1),
	AROS_LHA(BOOL,   killRequesters, D1),

/*  LOCATION */

	struct Library *, nvBase, 5, Nonvolatile)

/*  FUNCTION

    Search the nonvolatile storage for the object 'itemName' allocated by
    'appName' and return a copy of the data.

    INPUTS

    appName         --  name of the application that allocated the item
    itemName        --  the object to look for
    killRequesters  --  if TRUE no system requesters will be allowed to be
                        displayed during the operation of this function

    RESULT

    Pointer to the data assocated with 'itemName' as allocated by 'appName'.
    
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    FreeNVData(), <libraries/nonvolatile.h>

    INTERNALS

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
