/* AROS icclass implementation
 * 10/25/96 caldi@usa.nai.net
 */

#include <exec/types.h>

#include <dos/dos.h>
#include <dos/dosextens.h>

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <intuition/icclass.h>

#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>

#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <clib/macros.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#ifdef _AROS
#include <aros/asmcall.h>
#include <proto/alib.h>
#include "intuition_intern.h"
#endif

/****************************************************************************/


/****************************************************************************/

struct ICData
{
    Object	   * ic_Target;
    struct TagItem * ic_Mapping;
    struct TagItem * ic_CloneTags;
    ULONG	     ic_LoopCounter;
};

/****************************************************************************/

#undef IntuitionBase
#define IntuitionBase	 ((struct IntuitionBase *)(cl->cl_UserData))

#ifdef ICTARGET

/* This nifty routine (hopefully) allows us to send a IDCMP message from a boopsi gadget method.
 *
 * It does not seem like we can support ICTARGET_IDCMP until AROS has a real compatible intuition :)
 */
static struct IntuiMessage SendIDCMPUpdate( Class *cl, Object *o, struct opUpdate *msg, ULONG Class, UWORD Code, APTR IAddress )
{
    struct IntuiMessage *imsg;

    imsg = msg->opu_GInfo->gi_Window->MessageKey;

    while( imsg && !(imsg->Class & IDCMP_LONELYMESSAGE) )
    {
	imsg = imsg->SpecialLink;
    }

    if( !imsg )
    {
	if( imsg = AllocMem( sizeof( struct ExtIntuiMessage ), MEMF_CLEAR ) )
	{
	    imsg->SpecialLink = msg->opu_GInfo->gi_Window->MessageKey;
	    msg->opu_GInfo->gi_Window->MessageKey = imsg->SpecialLink;
	    imsg->ExecMessage.mn_ReplyPort = msg->opu_GInfo->gi_Window->WindowPort;
	}
    }

    if( imsg )
    {
	imsg->Class = Class;
	imsg->Code = Code;
	imsg->Qualifier = 0;
	imsg->IAddress = IAddress;
	imsg->MouseX = 0;
	imsg->MouseY = 0;
	imsg->Seconds = 0;
	imsg->Micros = 0;
	imsg->IDCMPWindow = msg->opu_GInfo->gi_Window;

	PutMsg( msg->opu_GInfo->gi_Window->UserPort, (struct Message *)imsg );
    }
    return( imsg );
}
#endif


/* Send update notification to target
 */
static ULONG notify_icclass(Class *cl, Object *o, struct opUpdate *msg)
{
    struct ICData * ic = INST_DATA(cl,o);

    if ( ic->ic_Target != NULL )
    {
	if ( (msg->opu_AttrList) && (msg->opu_GInfo) )
	{
	    ic->ic_LoopCounter += 1UL;

	    /* don't get caught in a circular notify target loop */
	    if ( ic->ic_LoopCounter == 1UL)
	    {
		if (ic->ic_Target != (Object *)ICTARGET_IDCMP)
		{
		    if ((ic->ic_CloneTags = CloneTagItems(msg->opu_AttrList)))
		    {
			if (ic->ic_Mapping != NULL)
			{
			    MapTags(ic->ic_CloneTags, ic->ic_Mapping, TRUE);
			}

			DoMethod( ic->ic_Target,
			    OM_UPDATE,
			    ic->ic_CloneTags,
			    msg->opu_GInfo,
			    msg->opu_Flags);

			FreeTagItems(ic->ic_CloneTags);
		    }
		}
#ifdef ICTARGET
		else
		{
		    if ( ic->ic_CloneTags = CloneTagItems(msg->opu_AttrList) )
		    {
			if (ic->ic_Mapping != NULL)
			{
			    MapTags(ic->ic_CloneTags, ic->ic_Mapping, TRUE);
			}

			SendIDCMPUpdate( cl, o, msg, IDCMP_IDCMPUPDATE, 0, ic->ic_CloneTags);

			/* NOTE: ReplyMsg() must cause FreeTagItems(imsg->IAddress)
			 * when freeing a  IDCMP_IDCMPUPDATE message!!
			 */
		    }
		}
#endif
	    }

	    ic->ic_LoopCounter -= 1UL;
	}
    }
    return(1UL);
}

/* icclass boopsi dispatcher
 */
AROS_UFH3(static IPTR, dispatch_icclass,
    AROS_UFHA(Class *,  cl,  A0),
    AROS_UFHA(Object *, o,   A2),
    AROS_UFHA(Msg,      msg, A1)
)
{
    IPTR retval = 0UL;
    struct ICData *ic;

    if (msg->MethodID != OM_NEW)
	ic = INST_DATA(cl, retval);

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
	/* set_icclass(cl, (Object*)retval, (struct opSet *)msg); */

	/* Fall through */

    case OM_SET:
	{
	    struct TagItem *tstate = ((struct opSet *)msg)->ops_AttrList;
	    struct TagItem *tag;

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
	}
	break;

    case OM_NOTIFY:
	retval = (IPTR)notify_icclass(cl, o, (struct opUpdate *)msg);
	break;

    case OM_DISPOSE:
	{
	    struct ICData *ic = INST_DATA(cl, o);

	    ic->ic_LoopCounter = 0UL;

	    if(ic->ic_CloneTags)
	    {
		FreeTagItems(ic->ic_CloneTags);
		ic->ic_CloneTags = NULL;
	    }
	}

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

#undef IntuitionBase

/****************************************************************************/

/* Initialize our image class. */
struct IClass *InitICClass (struct IntuitionBase * IntuitionBase)
{
    struct IClass *cl = NULL;

    /* This is the code to make the icclass...
    */
    if ( (cl = MakeClass(ICCLASS, ROOTCLASS, NULL, sizeof(struct ICData), 0)) )
    {
	cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_icclass);
	cl->cl_Dispatcher.h_SubEntry = NULL;
	cl->cl_UserData 	     = (IPTR)IntuitionBase;

	AddClass (cl);
    }

    return (cl);
}

