/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$
 
    Get a copy of monitors list
*/

#include <intuition/extensions.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include "intuition_intern.h"
#include "monitorclass_private.h"

/*****************************************************************************

    NAME */

#include <proto/intuition.h>

    AROS_LH1(Object **, GetMonitorList,

/*  SYNOPSIS */
         AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 161, Intuition)

/*  FUNCTION
	Obtain an array of monitorclass objects installed in the
	system

    INPUTS
	tags - an optional pointer to a taglist with additional options.
	       Currently only one tag is defined:

	       GMLA_DisplayID - list only monitors matching the given
				display ID

    RESULT
	A pointer to a NULL-terminated array of BOOPSI object pointers.
	This is a copy of internal list, you need to free it using
	FreeMonitorList()

    NOTES
	This function is compatible with MorphOS v2.

    EXAMPLE

    BUGS

    SEE ALSO
	FreeMonitorList() 

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    struct Library *UtilityBase = GetPrivIBase(IntuitionBase)->UtilityBase;
    struct MinNode *n;
    Object **res;
    ULONG num = 1;
    ULONG id = GetTagData(GMLA_DisplayID, INVALID_ID, tags);

    ObtainSemaphoreShared(&GetPrivIBase(IntuitionBase)->MonitorListSem);

    for (n = GetPrivIBase(IntuitionBase)->MonitorList.mlh_Head; n->mln_Succ; n = n->mln_Succ) {
	if ((id == INVALID_ID) || DoMethod((Object *)n, MM_CheckID, id))
	    num++;
    }

    res = AllocVec(num * sizeof(Object *), MEMF_ANY);

    if (res) {
	num = 0;
        for (n = GetPrivIBase(IntuitionBase)->MonitorList.mlh_Head; n->mln_Succ; n = n->mln_Succ) {
	    if ((id == INVALID_ID) || DoMethod((Object *)n, MM_CheckID, id))
		res[num++] = (Object *)n;
	}
	res[num] = NULL;
    }

    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->MonitorListSem);

    return res;

    AROS_LIBFUNC_EXIT
}
