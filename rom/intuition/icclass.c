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
#include "icclass.h"

#define DEBUG_IC(x) ;

/**********************************************************************************************/

#undef IntuitionBase

/**********************************************************************************************/

/* icclass boopsi dispatcher
 */
AROS_UFH3S(IPTR, dispatch_icclass,
           AROS_UFHA(Class *,  cl,  A0),
           AROS_UFHA(Object *, o,   A2),
           AROS_UFHA(Msg,      msg, A1)
          )
{
    AROS_USERFUNC_INIT

    struct IntuiBase 	*IntuitionBase = (struct IntuiBase *)cl->cl_UserData;
    struct ICData   	*ic = NULL;
    IPTR            	 retval = 0UL;

    DEBUG_IC(dprintf("dispatch_icclass: Cl 0x%lx o 0x%lx msg 0x%lx\n",cl,o,msg));

    if (msg->MethodID != OM_NEW) ic = INST_DATA(cl, o);

    switch(msg->MethodID)
    {
	case OM_NEW:
            DEBUG_IC(dprintf("dispatch_icclass: OM_NEW\n"));
            retval = DoSuperMethodA(cl, o, msg);

            if (!retval) break;

            ic = INST_DATA(cl, retval);

            /* set some defaults */
            ic->ic_Target    = NULL;
            ic->ic_Mapping   = NULL;
            ic->ic_CloneTags = NULL;

            /* Handle our special tags - overrides defaults */
            /* Fall through */

	case OM_SET:
        {
            struct TagItem *tstate = ((struct opSet *)msg)->ops_AttrList;
            struct TagItem *tag;

            DEBUG_IC(dprintf("dispatch_icclass: OM_SET\n"));
            while ((tag = NextTagItem(&tstate)))
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
            DEBUG_IC(dprintf("dispatch_icclass: OM_SET Map 0x%lx Target 0x%lx\n",ic->ic_Mapping,ic->ic_Target));
            break;
        }

	case OM_UPDATE: /* Maxon HotHelp says so and for example relied upon by modelclass */
            DEBUG_IC(dprintf("dispatch_icclass: OM_UPDATE\n"));
	case OM_NOTIFY:
            /* Send update notification to target
            */
            DEBUG_IC(dprintf("dispatch_icclass: OM_NOTIFY\n"));
            DEBUG_IC(dprintf("dispatch_icclass: DoNotify\n"));
            retval = DoNotify(cl, o, INST_DATA(cl, o), (struct opUpdate *)msg);
            DEBUG_IC(dprintf("dispatch_icclass: DoNotify done\n"));
            break;

	case OM_DISPOSE:
            DEBUG_IC(dprintf("dispatch_icclass: OM_DISPOSE\n"));
            FreeICData(INST_DATA(cl, o));
            DoSuperMethodA(cl, o, msg);
            break;

	case OM_GET:
            DEBUG_IC(dprintf("dispatch_icclass: OM_GET\n"));
            switch (((struct opGet *)msg)->opg_AttrID)
            {
        	case ICA_MAP:
        	    *((struct opGet *)msg)->opg_Storage = (ULONG)ic->ic_Mapping;
        	    break;

        	case ICA_TARGET:
        	    *((struct opGet *)msg)->opg_Storage = (ULONG)ic->ic_Target;
        	    break;
            }

            break;

            /*
        	NOTE: I current don't see the purpose of the ICM_* methods
        	this implementation could be WAY off base...

        	stegerg: well, it is for example needed by modeclass which is
        	a superclass of icclass.

        	IMPORTANT: ICM_SETLOOP, ICM_CLEARLOOP, ICM_CHECKLOOP are also
        	handled by gadgetclass: change here <-> change there!

            */

	case  ICM_SETLOOP:        /* set/increment loop counter    */
            ic->ic_LoopCounter += 1UL;
            DEBUG_IC(dprintf("dispatch_icclass: ICM_SETLOOP new LoopCounter %ld\n",ic->ic_LoopCounter));
            break;

	case  ICM_CLEARLOOP:    /* clear/decrement loop counter */
    	    ic->ic_LoopCounter -= 1UL;
            DEBUG_IC(dprintf("dispatch_icclass: ICM_CLEAR new LoopCounter %ld\n",ic->ic_LoopCounter));
            break;

	case  ICM_CHECKLOOP:    /* set/increment loop    */
            DEBUG_IC(dprintf("dispatch_icclass: ICM_CHECKLOOP new LoopCounter %ld\n",ic->ic_LoopCounter));
            retval = (IPTR)ic->ic_LoopCounter;
            break;

	default:
            retval = DoSuperMethodA(cl, o, msg);
            break;

    } /* switch */

    DEBUG_IC(dprintf("dispatch_icclass: retval 0x%lx\n",retval));
    return retval;

    AROS_USERFUNC_EXIT
}  /* dispatch_icclass */

#undef IntuitionBase

/**********************************************************************************************/

struct IClass *InitICClass (struct IntuitionBase *IntuitionBase)
{
    struct IClass *cl = NULL;

    /* This is the code to make the icclass...
    */
    if ( (cl = MakeClass(ICCLASS, ROOTCLASS, NULL, sizeof(struct ICData), 0)) )
    {
        cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_icclass);
        cl->cl_Dispatcher.h_SubEntry = NULL;
        cl->cl_UserData              = (IPTR)IntuitionBase;

        AddClass (cl);
    }

    return (cl);
}

/**********************************************************************************************/

