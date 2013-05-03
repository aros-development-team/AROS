#include <proto/intuition.h>

#include "intuition_intern.h"
#include "monitorclass_private.h"

/*
 * Private internal function.
 * Find a monitorclass object corresponsing to a given monitor ID.
 */
OOP_Object *FindMonitorNode(ULONG id, struct IntuitionBase *IntuitionBase)
{
    struct MinNode *n;
    OOP_Object *ret = NULL;

    ObtainSemaphoreShared(&GetPrivIBase(IntuitionBase)->MonitorListSem);

    for (n = GetPrivIBase(IntuitionBase)->MonitorList.mlh_Head; n->mln_Succ; n = n->mln_Succ)
    {
	if (DoMethod((Object *)n, MM_CheckID, id))
	{
	    ret = (OOP_Object *)n;
	    break;
	}
    }

    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->MonitorListSem);

    return ret;
}
