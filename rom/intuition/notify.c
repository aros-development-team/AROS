/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <exec/memory.h>
#include <intuition/icclass.h>
#include <intuition/classusr.h>
#include <intuition/cghooks.h>
#include <intuition/intuition.h>
#include "intuition_intern.h"
#include "notify.h"


VOID FreeICStuff(struct ICData *ic, struct IntuitionBase *IntuitionBase)
{
    ic->ic_LoopCounter = 0UL;

    if(ic->ic_CloneTags)
    {
	FreeTagItems(ic->ic_CloneTags);
	ic->ic_CloneTags = NULL;
    }
}

#undef IntuitionBase
#define IntuitionBase ((struct IntuitionBase *)cl->cl_UserData)

#ifdef ICTARGET

/* This nifty routine (hopefully) allows us to send a IDCMP message from a boopsi gadget method.
 *
 * It does not seem like we can support ICTARGET_IDCMP until AROS has a real compatible intuition :)
 */
static struct IntuiMessage *SendIDCMPUpdate( Class *cl, Object *o, struct opUpdate *msg, ULONG Class, UWORD Code, APTR IAddress )
{
    struct IntuiMessage *imsg;

    imsg = msg->opu_GInfo->gi_Window->MessageKey;

    while( imsg && !(imsg->Class & IDCMP_LONELYMESSAGE) )
    {
	imsg = imsg->SpecialLink;
    }

    if( !imsg )
    {
	if( (imsg = AllocMem( sizeof( struct ExtIntuiMessage ), MEMF_CLEAR )) != NULL )
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

ULONG DoNotification(	Class		*cl,
			Object		*o,
			struct ICData	*ic,
			struct opUpdate	*msg)

{
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
			ic->ic_CloneTags = NULL;
		    }
		}
#ifdef ICTARGET
		else
		{
		    if ( (ic->ic_CloneTags = CloneTagItems(msg->opu_AttrList)) != NULL )
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
