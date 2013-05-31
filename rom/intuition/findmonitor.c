/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id: $

    Internal monitor database functions
*/

#include <proto/intuition.h>

#include "intuition_intern.h"
#include "monitorclass_private.h"

/*
 * Find a monitorclass object corresponsing to a given monitor ID.
 */
void *FindMonitorNode(ULONG id, struct IntuitionBase *IntuitionBase)
{
    struct MinNode *n;
    void *ret = NULL;

    ObtainSemaphoreShared(&GetPrivIBase(IntuitionBase)->MonitorListSem);

    for (n = GetPrivIBase(IntuitionBase)->MonitorList.mlh_Head; n->mln_Succ; n = n->mln_Succ)
    {
	if (DoMethod((Object *)n, MM_CheckID, id))
	{
	    ret = n;
	    break;
	}
    }

    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->MonitorListSem);

    return ret;
}

/*
 * Find the best monitor node according to either modeid or name.
 * Used by MorphOS version OpenScreen().
 * From comments it can be suggested that MorphOS has a concept of "class"
 * of a display. And several drivers may match one display.
 * In AROS we don't have this concept. So, this function is very simple and just
 * looks up the object by either name or modeid. 'class' argument is ignored.
 */
void *FindBestMonitorNode(void *class, const char *name, ULONG modeid, struct IntuitionBase *IntuitionBase)
{
    void *ret;

    if (modeid == INVALID_ID)
    {
        ObtainSemaphoreShared(&GetPrivIBase(IntuitionBase)->MonitorListSem);

        ret = FindName((struct List *)&GetPrivIBase(IntuitionBase)->MonitorList, name);

        ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->MonitorListSem);
    }
    else
    {
        ret = FindMonitorNode(modeid, IntuitionBase);
    }

    return ret;
}
