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

/**********************************************************************************************/

#undef IntuitionBase
#define IntuitionBase   ((struct IntuiBase *)(cl->cl_UserData))

/**********************************************************************************************/

IPTR ModelClass__OM_NEW(Class *cl, Object *o, Msg msg)
{
    struct ModelData *data;
    
    if ((o = (Object *)DoSuperMethodA(cl, o, msg)))
    {
	data = INST_DATA(cl, o);

	NEWLIST(&data->memberlist);
    };
    
    return (IPTR)o;
}

IPTR ModelClass__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct ModelData *data = (struct ModelData *)INST_DATA(cl, o);

    for(;;)
    {
	/* free all member objects */

	Object *member, *objstate;
	ULONG method;

	objstate = (Object *)data->memberlist.mlh_Head;
	member = NextObject(&objstate);
	if (!member) break;

	method = OM_REMOVE;
	DoMethodA(member, (Msg)&method);

	DisposeObject(member);
	
    }
    
    return DoSuperMethodA(cl, o, msg);
}

IPTR ModelClass__OM_ADDMEMBER(Class *cl, Object *o, struct opMember *msg)
{
    struct ModelData *data = (struct ModelData *)INST_DATA(cl, o);
    struct opAddTail method;

    method.MethodID  = OM_ADDTAIL;
    method.opat_List = (struct List *)&data->memberlist;

    DoMethodA( msg->opam_Object, (Msg)&method);
    
    return (IPTR)0;
}

IPTR ModelClass__OM_REMMEMBER(Class *cl, Object *o, struct opMember *msg)
{
    STACKULONG method = OM_REMOVE;

    return DoMethodA(msg->opam_Object, (Msg)&method);
}

IPTR ModelClass__OM_UPDATE(Class *cl, Object *o, struct opUpdate *msg)
{
    struct ModelData *data = (struct ModelData *)INST_DATA(cl, o);
    
    /* send OM_UPDATE to all members without mapping the tags! */

    if (!IsListEmpty((struct List *)&data->memberlist))
    {
	STACKULONG method = ICM_CHECKLOOP;

	if (DoMethodA(o, (Msg)&method) == 0) /* avoid loops */
	{
	    struct TagItem *clonetags;

	    if ((clonetags = CloneTagItems(msg->opu_AttrList)))
	    {
		struct opUpdate  opu = *msg;
		Object          *member, *objstate;

		opu.MethodID     = OM_UPDATE; /* not OM_NOTIFY! */
		opu.opu_AttrList = clonetags;

		method = ICM_SETLOOP;
		DoMethodA(o, (Msg)&method);

		objstate = (Object *)data->memberlist.mlh_Head;
		while((member = NextObject(&objstate)))
		{
		    DoMethodA(member, (Msg)&opu);

		    /* in case the member object poked around in the taglist: */
		    RefreshTagItemClones(clonetags, msg->opu_AttrList);
		}

		method = ICM_CLEARLOOP;
		DoMethodA(o, (Msg)&method);

		FreeTagItems(clonetags);
	    }

	} /* if (DoMethod(o, ICM_CHECKLOOP) == 0) */

    } /* if (!IsListEmpty(&data->memberlist)) */

    /* modelclass is a superclass of icclass so not only the members are targets,
     * but possibly also the modelclass object itself could have an ICA_Target.
     * This is handled by the superclass */

    return DoSuperMethodA(cl, o, (Msg)msg);
}
