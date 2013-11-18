/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Internal monitor database functions
*/

#include <proto/graphics.h>
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

static void *FindMonitorByName(const char *name, struct IntuitionBase *IntuitionBase)
{
    void *ret;

    ObtainSemaphoreShared(&GetPrivIBase(IntuitionBase)->MonitorListSem);
    ret = FindName((struct List *)&GetPrivIBase(IntuitionBase)->MonitorList, name);
    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->MonitorListSem);

    return ret;
}

/*
 * Find the best monitor node according to either modeid or name.
 * From MorphOS comments it can be suggested that MorphOS has a concept of
 * display drivers family, to which several drivers may belong.
 * In AROS we don't have this concept. So, this function is very simple and just
 * looks up the object by either name or modeid. 'class' argument is ignored.
 */
void *FindBestMonitorNode(void *class, const char *name, ULONG modeid, struct IntuitionBase *IntuitionBase)
{
    void *ret;

    if (modeid == INVALID_ID)
    {
        ret = FindMonitorByName(name, IntuitionBase);
    }
    else
    {
        ret = FindMonitorNode(modeid, IntuitionBase);
    }

    return ret;
}

/*
 * Find the best monitor with 3D capabilities. Our "best" criteria is a special index,
 * actually showing how much hardware 3D modes we support.
 * This is purely experimental. Perhaps heuristics in OpenScreen() could be done better,
 * however currently we try to follow MorphOS way.
 */
void *FindBest3dMonitor(void *family, struct IntuitionBase *IntuitionBase)
{
    void *ret = NULL;
    ULONG maxidx = 0;
    struct MinNode *n;
    ULONG idx;

    ObtainSemaphoreShared(&GetPrivIBase(IntuitionBase)->MonitorListSem);

    for (n = GetPrivIBase(IntuitionBase)->MonitorList.mlh_Head; n->mln_Succ; n = n->mln_Succ)
    {
        idx = DoMethod((Object *)n, MM_Calc3dCapability);

        if (idx > maxidx)
        {
            maxidx = idx;
            ret = n;
        }
    }

    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->MonitorListSem);

    return ret;
}

/*
 * Find best matching mode ID for the given monitor.
 * The idea is simple. Convert name to ID then use BestModeIDA().
 */
ULONG FindBestModeID(const char *name, ULONG depth, ULONG width, ULONG height, struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    Object *obj = FindMonitorByName(name, IntuitionBase);

    if (obj)
    {
        struct TagItem tags[] =
        {
            {BIDTAG_MonitorID    , 0     },
            {BIDTAG_DesiredWidth , width },
            {BIDTAG_DesiredHeight, height},
            {BIDTAG_Depth        , depth },
            {TAG_DONE            , 0     }
        };

        GetAttr(MA_MonitorID, obj, &tags[0].ti_Data);
        return BestModeIDA(tags);
    }

    return INVALID_ID;
}
