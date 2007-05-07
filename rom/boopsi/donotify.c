/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
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
#include <proto/intuition.h>

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

*/
static struct IntuiMessage *SendIDCMPUpdate(
    Class 		*cl,
    Object 		*o,
    struct opUpdate	*msg, 
    ULONG		class,
    UWORD		code,
    APTR		IAddress,
    struct Library 	*BOOPSIBase)
{
    struct IntuiMessage	*imsg;

    imsg = AllocIntuiMessage(msg->opu_GInfo->gi_Window);

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

	/* done by AllocIntuiMessage
	imsg->IDCMPWindow = msg->opu_GInfo->gi_Window;
	*/
	
	SendIntuiMessage(msg->opu_GInfo->gi_Window , imsg);
	
    }
    
    return imsg;
}

/*****i***********************************************************************

    NAME */

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
		    	if (msg->opu_GInfo)
			if (msg->opu_GInfo->gi_Window)
			if (msg->opu_GInfo->gi_Window->UserPort)
			if (msg->opu_GInfo->gi_Window->IDCMPFlags & IDCMP_IDCMPUPDATE)
			{
			    struct TagItem 	*ti;
			    UWORD 		code = 0;
			    
			    if ((ti = FindTagItem(ICSPECIAL_CODE, ic->ic_CloneTags)))
			    {
			        code = ti->ti_Data & 0xFFFF;
			    } 
			    SendIDCMPUpdate( cl, o, msg, IDCMP_IDCMPUPDATE,
					     code, ic->ic_CloneTags, BOOPSIBase );

			    /* in this case the cloned tagitems will be freed in the Intuition
			       InputHandler when the app has replied the IntuiMessage */

			    ic->ic_CloneTags = NULL;
			}
			
			/* if IDCMP_IDCMPUPDATE msg could not be sent, free taglist */
			
			if (ic->ic_CloneTags)
			{
			    FreeTagItems(ic->ic_CloneTags);
			    ic->ic_CloneTags = NULL;
			}
			
		    }
		    
		} /* CloneTagItems() */

	    } /* LoopCounter == 1UL */

	    ic->ic_LoopCounter -= 1UL;

	} /* valid parameters */

    } /* valid target */

    return 1UL;

    AROS_LIBFUNC_EXIT
    
} /* DoNotify() */

