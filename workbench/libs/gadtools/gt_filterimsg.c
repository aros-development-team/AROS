/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include "gadtools_intern.h"

/*********************************************************************

    NAME */
#include <proto/gadtools.h>

	AROS_LH1(struct IntuiMessage *, GT_FilterIMsg,

/*  SYNOPSIS */
	AROS_LHA(struct IntuiMessage *, imsg, A1),

/*  LOCATION */
	struct Library *, GadToolsBase, 17, GadTools)

/*  FUNCTION
	Processes an intuition message. Normally, you should not use this
	function and call GT_GetIMsg() instead. If this functions returns
	with a value != NULL, you have to call GT_PostFilterIMsg(), when
	you are done with processing the message. If it return a NULL
	pointer, you have to ReplyMsg() the message, you passed to
	GT_FilterIMsg().

    INPUTS
	imsg - pointer to the intuition message to process

    RESULT
	Either a pointer to a processed intuition message or NULL, in which
	case the message had only meaning to gadtools.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	GT_PostFilterIMsg(), GT_GetIMsg(), intuition.library/ReplyMsg()

    INTERNALS

    HISTORY

***************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GadToolsBase_intern *,GadToolsBase)

    struct GT_IntuiMessage  *rc = NULL;
    struct GT_ContextGadget *contextgad;
    struct Gadget	    *gad;
    IPTR		    old_gadget_value;
    BOOL		    msg_only_for_gadtools = FALSE;
    
    /* Find Context Gadget. Should be first GTYP_GADTOOLS gadget
       in window´s gadgetlist */
    
    contextgad = (struct GT_ContextGadget *)imsg->IDCMPWindow->FirstGadget;
    while (contextgad)
    {
    	if (contextgad->gad.GadgetType == GTYP_GADTOOLS)
	{
	    if ((contextgad->magic  == CONTEXT_MAGIC) &&
	        (contextgad->magic2 == CONTEXT_MAGIC2))
	    {
	        break;
	    }
	}
	contextgad = (struct GT_ContextGadget *)contextgad->gad.NextGadget;
    }
    
    if (contextgad)
    {
	rc = &contextgad->gtmsg;
	
	/* copy imsg into extended gt intuimessage */
	
	rc->imsg = *(struct ExtIntuiMessage *)imsg;
	rc->origmsg = imsg;
	/* rc->wasalloced = FALSE; */
	
	switch(imsg->Class)
	{
	    case IDCMP_GADGETDOWN:
	    	contextgad->activegadget = NULL;
		
	    	gad = (struct Gadget *)imsg->IAddress;

		if ((gad->GadgetType & GTYP_GADTOOLS) &&
		    ((gad->GadgetType & GTYP_GTYPEMASK) == GTYP_CUSTOMGADGET))
		{
		    contextgad->activegadget = gad;
		    contextgad->gadgetkind = 0;
		    contextgad->childgadgetkind = 0;
		    contextgad->scrollticker = 3;
		    
		    GetAttr(GTA_GadgetKind, (Object *)gad, &contextgad->gadgetkind);
		    GetAttr(GTA_ChildGadgetKind, (Object *)gad, &contextgad->childgadgetkind);
		    
		    switch(contextgad->gadgetkind)
		    {
		    	case SCROLLER_KIND:
			case LISTVIEW_KIND:
			    msg_only_for_gadtools = TRUE;
			    switch(contextgad->childgadgetkind)
			    {
			    	case SCROLLER_KIND:
				    if (contextgad->gadgetkind == SCROLLER_KIND)
				    {
				    	/* listview gadgets don´t report scroller
					   activity to app */
					  
					contextgad->getattrtag = GTSC_Top;
		    	    		GetAttr(GTSC_Top, (Object *)gad, &contextgad->gadget_value);
					
					msg_only_for_gadtools = FALSE;
					
					rc->imsg.eim_IntuiMessage.Code = contextgad->gadget_value;
				    }
			    	    break;
				
				case _ARROW_KIND:
				{
				    struct TagItem settags[] =
				    {
				    	{0, 1},
					{TAG_DONE}
				    };			    
				    
				    contextgad->getattrtag = GTSC_Top;
				    
				    GetAttr(GTA_Arrow_Type, (Object *)gad, (IPTR *)&contextgad->childinfo);
				    GetAttr(GTA_Arrow_Scroller, (Object *)gad, (IPTR *)&contextgad->parentgadget);
				    GetAttr(GTSC_Top, (Object *)contextgad->parentgadget, &contextgad->gadget_value);
	
				    if (contextgad->childinfo == LEFTIMAGE || contextgad->childinfo == UPIMAGE)
				    {
				    	settags[0].ti_Tag = contextgad->setattrtag = GTA_Scroller_Dec;
				    }
				    else
				    {
				    	settags[0].ti_Tag = contextgad->setattrtag = GTA_Scroller_Inc;
				    }
				    
				    old_gadget_value = contextgad->gadget_value;

				    SetGadgetAttrsA(contextgad->parentgadget, imsg->IDCMPWindow, NULL, settags);

				    if (contextgad->gadgetkind == SCROLLER_KIND)
				    {
				    	/* nothing todo for listview kind */
					
					GetAttr(GTSC_Top, (Object *)contextgad->parentgadget, &contextgad->gadget_value);
					if (old_gadget_value != contextgad->gadget_value)
					{
				    	    msg_only_for_gadtools = FALSE;

					    rc->imsg.eim_IntuiMessage.Code = contextgad->gadget_value;
					    rc->imsg.eim_IntuiMessage.IAddress = (APTR)contextgad->parentgadget;
					}
				    }
				} /* case _ARROW_KIND: */
				break;
				
			    } /* switch(contextgad->childgadgetkind) */
			    break;
			
			case SLIDER_KIND:
			    contextgad->getattrtag = GTSL_Level;
			    GetAttr(GTSL_Level, (Object *)gad, &contextgad->gadget_value);
			    rc->imsg.eim_IntuiMessage.Code = contextgad->gadget_value;
			    break;
			
			case MX_KIND:
			    GetAttr(GTMX_Active, (Object *)gad, &contextgad->gadget_value);
			    rc->imsg.eim_IntuiMessage.Code = contextgad->gadget_value;
			    break;
			    
		    } /* switch(contextgad->gadgetkind) */
		    
		} /* if gadtools gadget and customgadget */ 
		break;
	
	    case IDCMP_GADGETUP:
	    	gad = (struct Gadget *)imsg->IAddress;
		if (gad == contextgad->activegadget)
		{
		    switch(contextgad->gadgetkind)
		    {
		    	case SCROLLER_KIND:
			case SLIDER_KIND:
			    msg_only_for_gadtools = TRUE;
			    
			    if (contextgad->gadgetkind == contextgad->childgadgetkind)
			    {
			    	/* scroller/slider gadget released,
				   not one of the arrow gadgets */

				msg_only_for_gadtools = FALSE;

				rc->imsg.eim_IntuiMessage.Code = contextgad->gadget_value;
			    }
			    break;

			case LISTVIEW_KIND:
			    if (contextgad->childgadgetkind != LISTVIEW_KIND)
			    {
			    	msg_only_for_gadtools = TRUE;
			    }
			    
			    break;
			    
		    } /* switch(contextgad->gadgetkind) */

		} /* if (gad == contextgad->activegadget) */

	    	contextgad->activegadget = NULL;
		break;
	
	    case IDCMP_INTUITICKS:		
	    	if (!(gad = contextgad->activegadget))
		{
		    contextgad->activegadget = NULL;
		} else if (gad->Activation & GACT_ACTIVEGADGET)
		{
		    if (contextgad->childgadgetkind == _ARROW_KIND)
		    {
		    	msg_only_for_gadtools = TRUE;
			
		        if (contextgad->scrollticker)
		        {
		    	    contextgad->scrollticker--;
		        }
			else
			{

			    if (gad->Flags & GFLG_SELECTED)
			    {
				struct TagItem settags[] =
				{
				    {contextgad->setattrtag, 1},
				    {TAG_DONE}
				};			    

		    		old_gadget_value = contextgad->gadget_value;

				SetGadgetAttrsA(contextgad->parentgadget, imsg->IDCMPWindow, NULL, settags);

				GetAttr(GTSC_Top, (Object *)contextgad->parentgadget, &contextgad->gadget_value);
	
				if (old_gadget_value != contextgad->gadget_value)
				{
				    msg_only_for_gadtools = FALSE;

				    rc->imsg.eim_IntuiMessage.Class = IDCMP_MOUSEMOVE;
				    rc->imsg.eim_IntuiMessage.Code = contextgad->gadget_value;
				    rc->imsg.eim_IntuiMessage.IAddress = (APTR)contextgad->parentgadget;
				}
				
			    } /* if (gad->Flags & GFLG_SELECTED) */

			}

		    } /* if (contextgad->childgadgetkind == _ARROW_KIND) */

		} /* else if (gad->Activation & GACT_ACTIVEGADGET) */
		break;
		
	    case IDCMP_MOUSEMOVE:
	    	if (!(gad = contextgad->activegadget))
		{
		    contextgad->activegadget = NULL;
		}
		else if (gad->Activation & GACT_ACTIVEGADGET)
		{
		    /* gadget is still active */

		    old_gadget_value = contextgad->gadget_value;
		    
		    switch (contextgad->gadgetkind)
		    {
			case SCROLLER_KIND:
			case SLIDER_KIND:
			    msg_only_for_gadtools = TRUE;
			    
			    if (contextgad->childgadgetkind == contextgad->gadgetkind)
			    {
		    	        GetAttr(contextgad->getattrtag,
			    	        (Object *)gad,
				        &contextgad->gadget_value);

			        if (contextgad->gadget_value != old_gadget_value)
			        {
				    msg_only_for_gadtools = FALSE;

				    rc->imsg.eim_IntuiMessage.IAddress = gad;
			    	    rc->imsg.eim_IntuiMessage.Code = contextgad->gadget_value;
			        }
			    }
			    break;
			
			case LISTVIEW_KIND:
			    msg_only_for_gadtools = TRUE;
			    break;
			    
		    } /* switch (contextgad->gadgetkind) */
		    
		} /* gadget is active */
		break;
		
	} /* switch (imsg->Class) */
	
	if (msg_only_for_gadtools)
	{
	    /* it´s not really necessary to call GT_PFIMsg here
	       , in the actual implementation */
	       
	    GT_PostFilterIMsg((struct IntuiMessage *)rc);
	    
	    /* this tells GT_GetImsg that imsg was for GadTools
	       use only = it calls ReplyMsg and does not hand
	       over the msg to the app */

	    rc = NULL;
	}
	
    } /* if (contextgad) */
    else
    {
    	/* no context gadget = stupid coder which uses
	   GT_GetIMsg without having gadtools gadgets
	   in the window. We must alloc a GT_IntuiMessage
	   because GT_PostFilterIMsg (which is called
	   by GT_ReplyMsg) relies on this extended structure */
	
	rc = AllocMem(sizeof(struct GT_IntuiMessage),MEMF_PUBLIC | MEMF_CLEAR);
	if (rc)
	{
	    /* copy imsg into extended gt intuimessage */
	    
	    rc->imsg = *(struct ExtIntuiMessage *)imsg;
	    rc->origmsg = imsg;
	    rc->wasalloced = TRUE;
	}
	
	/* if allocation fails, GT_GetIMsg thinks msg was for
	   GadTools use only and replies the origmsg. So no
	   bad things will happen. The only problem is that
	   the app will not get the msg. */
	    
    } /* if (contextgad) else */

    return (struct IntuiMessage *)rc;

    AROS_LIBFUNC_EXIT
    
} /* GT_FilterIMsg */
