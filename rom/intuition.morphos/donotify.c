/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
 
    ICClass notification support routines.
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/classes.h>
#include <intuition/cghooks.h>
#include <intuition/icclass.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include "intuition_intern.h"
#include "icclass.h"

#define DEBUG_NOTIFY(x) ;

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
static struct IntuiMessage *SendIDCMPUpdate(Class *cl, Object *o, struct opUpdate *msg,
                    	    	    	    ULONG class, UWORD code, APTR IAddress,
                    	    	    	    struct IntuitionBase *IntuitionBase)
{
    struct IntuiMessage *imsg;

    DEBUG_NOTIFY(dprintf("SendIDCMPUpdate:\n"));

    imsg = AllocIntuiMessage(msg->opu_GInfo->gi_Window);

    if( imsg )
    {
        imsg->Class 	= class;
        imsg->Code  	= code;
        imsg->Qualifier = (msg->opu_Flags == OPUF_INTERIM) ? ICMAGIC : 0; //tells SendIntuiMessage if the message is OK to be dropped
        imsg->IAddress  = IAddress;
        imsg->MouseX    = msg->opu_GInfo->gi_Window->MouseX;
        imsg->MouseY    = msg->opu_GInfo->gi_Window->MouseY;
        imsg->Seconds   = IntuitionBase->Seconds;
        imsg->Micros    = IntuitionBase->Micros;

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
         AROS_LHA(Class *,      cl, A0),
         AROS_LHA(Object *,     o,  A1),
         AROS_LHA(struct ICData *,  ic, A2),
         AROS_LHA(struct opUpdate *,    msg,    A3),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 145, Intuition)

/*  FUNCTION
    This function provides a way for icclass objects to notify
    their listeners when they are notifying. It is mainly
    provided as an external function for intuition.library's
    gadgetclass implementation, which contains an inbuilt
    icclass.
 
    INPUTS
        cl      - my class
        o       - this object
        icdata  - interconnection information
        msg     - the message given to the OM_NOTIFY method
 
    RESULT
    The objects listening to this object will be notified.
 
    Note: Return value not clear.
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *, IntuitionBase)

    DEBUG_NOTIFY(dprintf("DoNotify: cl 0x%lx o 0x%lx ICData 0x%lx opUpdate 0x%lx\n",cl,o,ic,msg));

    SANITY_CHECKR(o,1UL)
    SANITY_CHECKR(cl,1UL)
    SANITY_CHECKR(msg,1UL)
    SANITY_CHECKR(ic,1UL)
    
    if( ic->ic_Target != NULL )
    {
        if( msg->opu_AttrList) /* stegerg: ??? checked also "&& msg->opu_GInfo" ) */
        {
            ic->ic_LoopCounter += 1UL;

            DEBUG_NOTIFY(dprintf("DoNotify: Loop counter %ld\n",ic->ic_LoopCounter));
            /* Don't get into a circular notify target loop */
            if( ic->ic_LoopCounter == 1UL )
            {
                if(( ic->ic_CloneTags = CloneTagItems(msg->opu_AttrList)))
                {
                    DEBUG_NOTIFY(dprintf("DoNotify: CloneTags 0x%lx\n",ic->ic_CloneTags));
                    if( ic->ic_Mapping != NULL )
                    {
                        MapTags(ic->ic_CloneTags, ic->ic_Mapping, MAP_KEEP_NOT_FOUND);
                    }

                    if( ic->ic_Target != (Object *)ICTARGET_IDCMP)
                    {
                        struct opUpdate opu;

                        opu.MethodID     = OM_UPDATE;
                        opu.opu_AttrList = ic->ic_CloneTags;
                        opu.opu_GInfo    = msg->opu_GInfo;
                        opu.opu_Flags    = msg->opu_Flags;

                        DEBUG_NOTIFY(dprintf("DoNotify: Send OM_UPDATA to 0x%lx\n",ic->ic_Target));
                        DoMethodA( ic->ic_Target, (Msg)&opu );
                    }
                    else
                    {
                        if (msg->opu_GInfo)
			{
                            if (msg->opu_GInfo->gi_Window)
			    {
                                if (msg->opu_GInfo->gi_Window->UserPort)
				{
                                    if (msg->opu_GInfo->gi_Window->IDCMPFlags & IDCMP_IDCMPUPDATE)
                                    {
                                        struct TagItem  *ti;
                                        UWORD       	 code = 0;

                                        if ((ti = FindTagItem(ICSPECIAL_CODE, ic->ic_CloneTags)))
                                        {
                                            code = ti->ti_Data & 0xFFFF;
                                        }
                                        SendIDCMPUpdate( cl, o, msg, IDCMP_IDCMPUPDATE,
                                                         code, ic->ic_CloneTags, IntuitionBase );

                                        /* in this case the cloned tagitems will be freed in the Intuition
                                           InputHandler when the app has replied the IntuiMessage */

                                        ic->ic_CloneTags = NULL;
                                    }
				}
			    }
			}
                    }

                    FreeTagItems(ic->ic_CloneTags);

                    ic->ic_CloneTags = NULL;

                } /* CloneTagItems() */

            } /* LoopCounter == 1UL */
            else
            {
                DEBUG_NOTIFY(dprintf("DoNotify: skip..circular\n"));
            }

            ic->ic_LoopCounter -= 1UL;

        } /* valid parameters */
        else
        {
            DEBUG_NOTIFY(dprintf("DoNotify: no AttrList\n"));
        }
    } /* valid target */
    else
    {
        DEBUG_NOTIFY(dprintf("DoNotify: No Target\n"));
    }
    return 1UL;

    AROS_LIBFUNC_EXIT
} /* DoNotify() */

