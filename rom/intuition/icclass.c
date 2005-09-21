/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <exec/types.h>

#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <intuition/icclass.h>

#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <clib/macros.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#ifndef __MORPHOS__
#include <aros/asmcall.h>
#include <proto/alib.h>
#endif /* !__MORPHOS__ */

#include "intuition_intern.h"

#define DEBUG_IC(x) ;

/**********************************************************************************************/

static void _om_set(struct ICData *ic, struct TagItem *tags, struct IntuitionBase *IntuitionBase)
{
    struct TagItem *tag, *tstate = tags;

    while ((tag = NextTagItem((const struct TagItem **)&tstate)))
    {
	switch(tag->ti_Tag)
	{
	case ICA_MAP:
	    ic->ic_Mapping = (struct TagItem *)tag->ti_Data;
	    break;

	case ICA_TARGET:
	    ic->ic_Target = (Object *)tag->ti_Data;
	    break;
	}
    }
}

IPTR ICClass__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;
    struct ICData *ic;
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    
    DEBUG_IC(dprintf("dispatch_icclass: OM_NEW\n"));
    
    retval = DoSuperMethodA(cl, o, (Msg)msg);

    if (!retval) return (IPTR)FALSE;

    ic = INST_DATA(cl, retval);

    /* set some defaults */
    ic->ic_Target    = NULL;
    ic->ic_Mapping   = NULL;
    ic->ic_CloneTags = NULL;

    _om_set(ic, msg->ops_AttrList, IntuitionBase);
    
    return retval;
}

IPTR ICClass__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    struct ICData *ic = INST_DATA(cl, o);
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;

    DEBUG_IC(dprintf("dispatch_icclass: OM_SET\n"));
    _om_set(ic, msg->ops_AttrList, IntuitionBase);
    DEBUG_IC(dprintf("dispatch_icclass: OM_SET Map 0x%lx Target 0x%lx\n",ic->ic_Mapping,ic->ic_Target));
    
    return (IPTR)TRUE;
}

IPTR ICClass__OM_NOTIFY(Class *cl, Object *o, struct opUpdate *msg)
{
    struct ICData *ic = INST_DATA(cl, o);
    IPTR retval;
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    
    /* Send update notification to target
     */
    DEBUG_IC(dprintf("dispatch_icclass: OM_NOTIFY\n"));
    DEBUG_IC(dprintf("dispatch_icclass: DoNotify\n"));
    retval = DoNotify(cl, o, ic, msg);
    DEBUG_IC(dprintf("dispatch_icclass: DoNotify done\n"));
    
    return retval;
}

IPTR ICClass__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct ICData *ic = INST_DATA(cl, o);
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;

    DEBUG_IC(dprintf("dispatch_icclass: OM_DISPOSE\n"));
    FreeICData(ic);
    DoSuperMethodA(cl, o, msg);
    
    return (IPTR)TRUE;
}

IPTR ICClass__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct ICData *ic = INST_DATA(cl, o);

    DEBUG_IC(dprintf("dispatch_icclass: OM_GET\n"));
    switch (msg->opg_AttrID)
    {
    case ICA_MAP:
	*msg->opg_Storage = (ULONG)ic->ic_Mapping;
	break;

    case ICA_TARGET:
	*msg->opg_Storage = (ULONG)ic->ic_Target;
	break;
    }

    return (IPTR)TRUE;
}


/*
 * NOTE: I current don't see the purpose of the ICM_* methods
 * this implementation could be WAY off base...
 *
 * stegerg: well, it is for example needed by modeclass which is
 * a superclass of icclass.
 *
 * IMPORTANT: ICM_SETLOOP, ICM_CLEARLOOP, ICM_CHECKLOOP are also
 * handled by gadgetclass: change here <-> change there!
 */

void ICClass__ICM_SETLOOP(Class *cl, Object *o, Msg msg)
{
    struct ICData *ic = INST_DATA(cl, o);

    /* set/increment loop counter    */
    ic->ic_LoopCounter += 1UL;
    DEBUG_IC(dprintf("dispatch_icclass: ICM_SETLOOP new LoopCounter %ld\n",ic->ic_LoopCounter));
}

void ICClass__ICM_CLEARLOOP(Class *cl, Object *o, Msg msg)
{
    struct ICData *ic = INST_DATA(cl, o);
    
    /* clear/decrement loop counter */
    ic->ic_LoopCounter -= 1UL;
    DEBUG_IC(dprintf("dispatch_icclass: ICM_CLEAR new LoopCounter %ld\n",ic->ic_LoopCounter));
}

IPTR ICClass__ICM_CHECKLOOP(Class *cl, Object *o, Msg msg)
{
    struct ICData *ic = INST_DATA(cl, o);

    /* get loop counter   */
    DEBUG_IC(dprintf("dispatch_icclass: ICM_CHECKLOOP new LoopCounter %ld\n",ic->ic_LoopCounter));
    return (IPTR)ic->ic_LoopCounter;
}

/**********************************************************************************************/

