/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dosextens.h>

/*****************************************************************************

    NAME */

#include <exec/lists.h>
#include <libraries/nonvolatile.h>
#include <proto/exec.h>
#include <proto/nvdisk.h>
#include "nonvolatile_intern.h"

#include <string.h>


AROS_LH2(struct MinList *, GetNVList,

/*  SYNOPSIS */

	AROS_LHA(STRPTR, appName,        A0),
	AROS_LHA(BOOL,   killRequesters, D1),

/*  LOCATION */

	struct Library *, nvBase, 10, Nonvolatile)

/*  FUNCTION

    Returns a list of items allocated by application 'appName'.

    INPUTS

    appName         --  the application the nonvolatile items of which to query
                        about
    killRequesters  --  if TRUE you make sure that no system requesters will be
                        displayed during the operation of this function

    RESULT

    Pointer to a MinList of NVEntries which describes the items. Failure due to
    lack of memory will be indicated by returning NULL.
    
    NOTES

    The protection field should be examined using the field masks NVIF_DELETE
    or by the bit definition NVIB_DELETE as the other bits are reserved for
    system use.

    EXAMPLE

    BUGS

    SEE ALSO

    FreeNVData(), SetNVProtection(), <libraries/nonvolatile.h>

    INTERNALS

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    struct Process *me = (struct Process *)FindTask(NULL);
    APTR oldReq = me->pr_WindowPtr;
    struct MinList *list;

    if(appName == NULL)
	return NULL;

    if(strpbrk(appName, ":/") != NULL)
	return NULL;

    if(killRequesters)
	me->pr_WindowPtr = (APTR)-1;

    list = GetItemList(appName);

    if(killRequesters)
	me->pr_WindowPtr = oldReq;

    return list;

    AROS_LIBFUNC_EXIT
} /* GetNVList */
