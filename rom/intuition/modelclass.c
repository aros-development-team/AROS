/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
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

struct ModelData
{
    struct MinList memberlist;
};

/**********************************************************************************************/

/* icclass boopsi dispatcher
 */
AROS_UFH3S(IPTR, dispatch_modelclass,
           AROS_UFHA(Class *,  cl,  A0),
           AROS_UFHA(Object *, o,   A2),
           AROS_UFHA(Msg,      msg, A1)
          )
{
    AROS_USERFUNC_INIT

    struct ModelData *data = NULL;   /* shut up the compiler */
    IPTR              retval = 0UL;

    if (msg->MethodID != OM_NEW) data = INST_DATA(cl, o);

    switch(msg->MethodID)
    {
	case OM_NEW:
            if ((o = (Object *)DoSuperMethodA(cl, o, msg)))
            {
        	data = INST_DATA(cl, o);

        	NEWLIST(&data->memberlist);

        	retval = (IPTR)o;
            };
            break;

	case OM_DISPOSE:
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
            retval = DoSuperMethodA(cl, o, msg);
            break;

	case OM_ADDMEMBER:
        {
            struct opAddTail method;

            method.MethodID  = OM_ADDTAIL;
            method.opat_List = (struct List *)&data->memberlist;

            DoMethodA( ((struct opMember *)msg)->opam_Object, (Msg)&method);
            break;
        }

	case OM_REMMEMBER:
        {
            STACKULONG method = OM_REMOVE;

            DoMethodA( ((struct opMember *)msg)->opam_Object, (Msg)&method);
            break;
        }

	case OM_UPDATE:
	case OM_NOTIFY:
            /* send OM_UPDATE to all members without mapping the tags! */

            if (!IsListEmpty((struct List *)&data->memberlist))
            {
        	STACKULONG method = ICM_CHECKLOOP;

        	if (DoMethodA(o, (Msg)&method) == 0) /* avoid loops */
        	{
                    struct TagItem *clonetags;

                    if ((clonetags = CloneTagItems(((struct opUpdate *)msg)->opu_AttrList)))
                    {
                	struct opUpdate  opu = *(struct opUpdate *)msg;
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
                            RefreshTagItemClones(clonetags, ((struct opUpdate *)msg)->opu_AttrList);
                	}

                	method = ICM_CLEARLOOP;
                	DoMethodA(o, (Msg)&method);

                	FreeTagItems(clonetags);
                    }

        	} /* if (DoMethod(o, ICM_CHECKLOOP) == 0) */

            } /* if (!IsListEmpty(&data->memberlist)) */

            /* modelclass is a superclass of icclass so not only the members are targets,
               but possibly also the modelclass object itself could have an ICA_Target.
               This is handled by the superclass */

            retval = DoSuperMethodA(cl, o, msg);
            break;

	default:
            retval = DoSuperMethodA(cl, o, msg);
            break;

    } /* switch(msg->MethodID) */

    return retval;

    AROS_USERFUNC_EXIT
}  /* dispatch_modelclass */

#undef IntuitionBase

/**********************************************************************************************/

struct IClass *InitModelClass (struct IntuitionBase *IntuitionBase)
{
    struct IClass *cl = NULL;

    /* This is the code to make the modelclass...
    */
    if ( (cl = MakeClass(MODELCLASS, ICCLASS, NULL, sizeof(struct ModelData), 0)) )
    {
        cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_modelclass);
        cl->cl_Dispatcher.h_SubEntry = NULL;
        cl->cl_UserData              = (IPTR)IntuitionBase;

        AddClass (cl);
    }

    return (cl);
}

/**********************************************************************************************/

