/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: icclass implementation.
    Lang: english
*/

/* AROS icclass implementation
 * 10/25/96 caldi@usa.nai.net
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
#include <proto/boopsi.h>
#include <clib/intuition_protos.h>	/* UgH - Need DoMethod() etc */

#ifdef __AROS__
#include <aros/asmcall.h>
#include <proto/alib.h>
#endif

#include "intern.h"

/**********************************************************************************************/

#undef BOOPSIBase
#define BOOPSIBase	((struct Library *)(cl->cl_UserData))

/**********************************************************************************************/

/* icclass boopsi dispatcher
 */
AROS_UFH3S(IPTR, dispatch_icclass,
    AROS_UFHA(Class *,  cl,  A0),
    AROS_UFHA(Object *, o,   A2),
    AROS_UFHA(Msg,      msg, A1)
)
{
    struct ICData	*ic = NULL;
    IPTR 		retval = 0UL;

    if (msg->MethodID != OM_NEW)
	ic = INST_DATA(cl, o);

    switch(msg->MethodID)
    {
	case OM_NEW:
	    retval = DoSuperMethodA(cl, o, msg);

	    if (!retval)
		break;

	    ic = INST_DATA(cl, retval);

	    /* set some defaults */
	    ic->ic_Target	 = NULL;
	    ic->ic_Mapping	 = NULL;
	    ic->ic_CloneTags = NULL;

	    /* Handle our special tags - overrides defaults */
	    /* Fall through */

	case OM_SET:
	    {
		struct TagItem *tstate = ((struct opSet *)msg)->ops_AttrList;
		struct TagItem *tag;

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
	    break;

	case OM_UPDATE: /* Maxon HotHelp says so and for example relied upon by modelclass */
	case OM_NOTIFY:
	   /* Send update notification to target
	   */
	    retval = DoNotify(cl, o, INST_DATA(cl, o), (struct opUpdate *)msg);
	    break;

	case OM_DISPOSE:
	    FreeICData(INST_DATA(cl, o));
	    DoSuperMethodA(cl, o, msg);
	    break;

	case OM_GET:
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

	case  ICM_SETLOOP:	      /* set/increment loop counter    */
	    {
		struct ICData *ic = INST_DATA(cl, o);

		ic->ic_LoopCounter += 1UL;
	    }

	    break;

	case  ICM_CLEARLOOP:    /* clear/decrement loop counter */
	    {
		struct ICData *ic = INST_DATA(cl, o);

		ic->ic_LoopCounter -= 1UL;
	    }

	    break;

	case  ICM_CHECKLOOP:    /* set/increment loop	 */
	    {
		struct ICData *ic = INST_DATA(cl, o);

		retval = (IPTR)ic->ic_LoopCounter;
	    }

	    break;

	default:
	    retval = DoSuperMethodA(cl, o, msg);
	    break;
	    
    } /* switch */

    return retval;
    
}  /* dispatch_icclass */

#undef BOOPSIBase

/**********************************************************************************************/

struct IClass *InitICClass (struct Library * BOOPSIBase)
{
    struct IClass *cl = NULL;

    /* This is the code to make the icclass...
    */
    if ( (cl = MakeClass(ICCLASS, ROOTCLASS, NULL, sizeof(struct ICData), 0)) )
    {
	cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_icclass);
	cl->cl_Dispatcher.h_SubEntry = NULL;
	cl->cl_UserData 	     = (IPTR)BOOPSIBase;

	AddClass (cl);
    }

    return (cl);
}

/**********************************************************************************************/

