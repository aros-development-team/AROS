/*
    Copyright (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: ICClass notification support routines.
    Lang: english
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/classes.h>
#include <intuition/cghooks.h>
#include <intuition/icclass.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/boopsi.h>
#include <clib/intuition_protos.h> /* for DoMethod() */

#include "intern.h"

/*
    Note: This file is essentially the contents of the file 
    rom/intuition/notify.c which contained code used by the icclass
    and gadgetclass in caldi's first implementation. It was split
    by iaint for the new boopsi.library.
*/


/*
    This will hopefully allow us to send an IDCMP message from a boopsi
    gadget method. 

    We may not be able to get this to work though, as AROS doesn't yet
    have a compatible intuition.
*/
static struct IntuiMessage *SendIDCMPUpdate(
    Class 		*cl,
    Object 		*o,
    struct opUpdate	*msg, 
    ULONG		class,
    UWORD		code,
    APTR		IAddress,
    struct Library *	BOOPSIBase
    )
{
    struct IntuiMessage	*imsg;

    imsg = msg->opu_GInfo->gi_Window->MessageKey;
    while( imsg && !(imsg->Class & IDCMP_LONELYMESSAGE) )
    {
	imsg = imsg->SpecialLink;
    }

    if( !imsg )
    {
	imsg = AllocMem(sizeof(struct ExtIntuiMessage), MEMF_CLEAR);
	if( imsg != NULL )
	{
	    /* Add the newly created message to start of list of messages */
	    imsg->SpecialLink = msg->opu_GInfo->gi_Window->MessageKey;
	    msg->opu_GInfo->gi_Window->MessageKey = imsg;
	    imsg->ExecMessage.mn_ReplyPort = msg->opu_GInfo->gi_Window->WindowPort;
	    
	}
    }

    if( imsg )
    {
	imsg->Class	= class;
	imsg->Code	= code;
	imsg->Qualifier = 0;
	imsg->IAddress	= IAddress;
	imsg->MouseX	= 0;
	imsg->MouseY	= 0;
	imsg->Seconds	= 0;
	imsg->Micros	= 0;
	imsg->IDCMPWindow = msg->opu_GInfo->gi_Window;
	
	PutMsg( msg->opu_GInfo->gi_Window->UserPort, (struct Message *)msg);
    }
    return imsg;
}

/*****i***********************************************************************

    NAME */
#include <boopsi_private.h>

	AROS_LH4(IPTR, DoNotify,

/*  SYNOPSIS */
	AROS_LHA(Class *,		cl,	A0),
	AROS_LHA(Object *,		o,	A1),
	AROS_LHA(struct ICData *,	ic,	A2),
	AROS_LHA(struct opUpdate *,	msg,	A3),

/*  LOCATION */
	struct Library *, BOOPSIBase, 16, BOOPSI)

/*  FUNCTION
	This function provides a way for icclass objects to notify 
	their listeners when they are notifying. It is mainly 
	provided as an external function for intuition.library's
	gadgetclass implementation, which contains an inbuilt 
	icclass.

    INPUTS
	    cl	    - my class
	    o	    - this object
	    icdata  - interconnection information
	    msg	    - the message given to the OM_NOTIFY method

    RESULT
	The objects listening to this object will be notified.

	Note: Return value not clear.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, BOOPSIBase)

    if( ic->ic_Target != NULL )
    {
	if( msg->opu_AttrList) /* stegerg: ??? checked also "&& msg->opu_GInfo" ) */
	{
	    ic->ic_LoopCounter += 1UL;

	    /* Don't get into a circular notify target loop */
	    if( ic->ic_LoopCounter == 1UL )
	    {
		if(( ic->ic_CloneTags = CloneTagItems(msg->opu_AttrList)))
		{
		    if( ic->ic_Mapping != NULL )
		    {
			MapTags(ic->ic_CloneTags, ic->ic_Mapping, TRUE);
		    }

		    if( ic->ic_Target != (Object *)ICTARGET_IDCMP)
		    {
			DoMethod( ic->ic_Target,
			    OM_UPDATE,
			    ic->ic_CloneTags,
			    msg->opu_GInfo,
			    msg->opu_Flags);
			
			FreeTagItems(ic->ic_CloneTags);
			ic->ic_CloneTags = NULL;
		    }
    		    else
    		    {
			SendIDCMPUpdate( cl, o, msg, IDCMP_IDCMPUPDATE,
					0, ic->ic_CloneTags, BOOPSIBase );
		    }
		} /* CloneTagItems() */

		ic->ic_LoopCounter -= 1UL;

	    } /* LoopCounter == 1UL */
	} /* valid parameters */
    } /* valid target */

    return 1UL;

    AROS_LIBFUNC_EXIT
} /* DoNotify() */

