/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition's InputHandler
    Lang: english
*/

/****************************************************************************************/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/alib.h>
#include <proto/layers.h>
#include <proto/graphics.h>
#include <proto/keymap.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include <exec/alerts.h>
#include <exec/interrupts.h>
#include <exec/ports.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <intuition/sghooks.h>
#include <devices/inputevent.h>
#include <devices/rawkeycodes.h>
#include "inputhandler.h"

#include "boopsigadgets.h"
#include "boolgadgets.h"
#include "propgadgets.h"
#include "strgadgets.h"
#include "gadgets.h"
#include "intuition_intern.h" /* EWFLG_xxx */
#include "maybe_boopsi.h"
#include "inputhandler_support.h"
#include "inputhandler_actions.h"
#include "menus.h"

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/


struct Interrupt *InitIIH(struct IntuitionBase *IntuitionBase)
{
    struct Interrupt *iihandler;
    struct IIHData   *iihdata;

    D(bug("InitIIH(IntuitionBase=%p)\n", IntuitionBase));

    iihandler = AllocMem(sizeof (struct Interrupt), MEMF_PUBLIC | MEMF_CLEAR);
    if (iihandler)
    {
	iihdata = AllocMem(sizeof (struct IIHData), MEMF_PUBLIC | MEMF_CLEAR);
	if (iihdata)
	{
	    struct MsgPort *port;
	    
	    port = AllocMem(sizeof (struct MsgPort), MEMF_PUBLIC | MEMF_CLEAR);
	    if (port)
	    {
	        if ((iihdata->InputEventMemPool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR,
							     sizeof(struct GeneratedInputEvent) * 10,
							     sizeof(struct GeneratedInputEvent) * 10)))
		{
		    struct TagItem dragtags[] =
		    {
		    	{GA_SysGadget	, TRUE	    	},
			{GA_SysGType	, GTYP_WDRAGGING},
			{TAG_DONE   	    	    	}
		    };
		    struct TagItem sizetags[] =
		    {
		    	{GA_SysGadget	, TRUE	    	},
			{GA_SysGType	, GTYP_SIZING	},
			{TAG_DONE   	    	    	}
		    };
		    
		    iihdata->MasterDragGadget = (struct Gadget *)NewObjectA(GetPrivIBase(IntuitionBase)->dragbarclass,
		    	    	    	    	    			    NULL,
									    dragtags);

		    iihdata->MasterSizeGadget = (struct Gadget *)NewObjectA(GetPrivIBase(IntuitionBase)->sizebuttonclass,
		    	    	    	    	    			    NULL,
									    sizetags);
		    			    			   
		    if (iihdata->MasterDragGadget && iihdata->MasterSizeGadget)
		    {
	        	ULONG lock;

			/* We do not want to be woken up by message replies.
			   We are anyway woken up about 10 times a second by
			   timer events
			*/
	    		port->mp_Flags   = PA_IGNORE;

	    		NEWLIST( &(port->mp_MsgList) );
	    		iihdata->IntuiReplyPort = port;

			NEWLIST(&iihdata->IntuiActionQueue);
    			NEWLIST(&iihdata->GeneratedInputEventList);

			/* Note: there are several routines like CloseWindow, which
			   expect is_Data to point to the IIHData structure, so don't
			   change this! */

			iihandler->is_Code = (APTR)AROS_ASMSYMNAME(IntuiInputHandler);
			iihandler->is_Data = iihdata;
			iihandler->is_Node.ln_Pri	= 50;
			iihandler->is_Node.ln_Name	= "Intuition InputHandler";

			lock = LockIBase(0UL);

			iihdata->IntuitionBase = IntuitionBase;

			UnlockIBase(lock);

			GetPrivIBase(IntuitionBase)->IntuiReplyPort = iihdata->IntuiReplyPort;
			GetPrivIBase(IntuitionBase)->IntuiActionQueue = &iihdata->IntuiActionQueue;

			ReturnPtr ("InitIIH", struct Interrupt *, iihandler);
			
		    } /* f (iihdata->MasterDragGadget && iihdata->MasterSizeGadget) */
		    
		    if (iihdata->MasterDragGadget) DisposeObject((Object *)iihdata->MasterDragGadget);
		    if (iihdata->MasterSizeGadget) DisposeObject((Object *)iihdata->MasterSizeGadget);
		    
		} /* if (iihdata->InputEventMemPool = ... */
		FreeMem(port, sizeof(struct MsgPort));
		
	    } /* if (port) */
	    FreeMem(iihdata, sizeof (struct IIHData));
	    
	} /* if (iihdata) */
	FreeMem(iihandler, sizeof (struct Interrupt));
	
    } /* if (iihandler) */
    
    ReturnPtr ("InitIIH", struct Interrupt *, NULL);
}

/****************************************************************************************/

VOID CleanupIIH(struct Interrupt *iihandler, struct IntuitionBase *IntuitionBase)
{
    struct IIHData *iihdata = (struct IIHData *)iihandler->is_Data;
    
    DisposeObject((Object *)iihdata->MasterDragGadget);
    DisposeObject((Object *)iihdata->MasterSizeGadget);
    
    FreeGeneratedInputEvents(iihdata);
    DeletePool(iihdata->InputEventMemPool);
    
    /* One might think that this port is still in use by the inputhandler.
    ** However, if intuition is closed for the last time, there should be no
    ** windows that IntuiMessage can be sent to.
    */
    FreeMem(iihdata->IntuiReplyPort, sizeof (struct MsgPort));
        
    FreeMem(iihdata, sizeof (struct IIHData));
    FreeMem(iihandler, sizeof (struct Interrupt));

    return;
}

/****************************************************************************************/

static void HandleIntuiReplyPort(struct IIHData *iihdata, struct IntuitionBase *IntuitionBase)
{
    struct IntuiMessage *im;
    
    while ((im = (struct IntuiMessage *)GetMsg(iihdata->IntuiReplyPort)))
    {
    	switch(im->Class)
	{
	    case IDCMP_MOUSEMOVE:
	    	IW(im->IDCMPWindow)->num_mouseevents--;
	    	break;
	    
	    case IDCMP_INTUITICKS:
	    	Forbid();
		im->IDCMPWindow->Flags &= ~WFLG_WINDOWTICKED;
		Permit();
	    	break;
	
	    case IDCMP_IDCMPUPDATE:
		if (im->IAddress)
		{
		    FreeTagItems((struct TagItem *)im->IAddress);
		}
		break;
		
	} /* switch(im->Class) */
		
	if (im->Qualifier & IEQUALIFIER_REPEAT)
	{
	    /* IDCMP_IDCMPUPDATE messages can also be sent from app task, therefore
	       it would be better if there was an ATOMIC_DEC macro or something */

	    if (IW(im->IDCMPWindow)->num_repeatevents)
	    {
	        IW(im->IDCMPWindow)->num_repeatevents--;
	    }
	}
			
    	FreeIntuiMessage(im);
	
    } /* while ((im = (struct IntuiMessage *)GetMsg(iihdata->IntuiReplyPort))) */
}

/****************************************************************************************/

AROS_UFH2(struct InputEvent *, IntuiInputHandler,
    AROS_UFHA(struct InputEvent *,      oldchain,       A0),
    AROS_UFHA(struct IIHData *,         iihdata,        A1)
)
{
    AROS_USERFUNC_INIT

    struct InputEvent 	    *ie, *orig_ie, stackie;
    struct Gadget   	    *gadget = iihdata->ActiveGadget;
    struct IntuitionBase    *IntuitionBase = iihdata->IntuitionBase;
    ULONG   	    	    lock;
    struct GadgetInfo 	    *gi = &iihdata->GadgetInfo;
    BOOL    	    	    reuse_event = FALSE;
    struct Window   	    *w;
#if SINGLE_SETPOINTERPOS_PER_EVENTLOOP
    WORD    	    	    pointerposx, pointerposy;
    BOOL    	    	    call_setpointerpos = FALSE;
#endif
 
    D(bug("Inside intuition inputhandler, active window=%p\n", IntuitionBase->ActiveWindow));

    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->InputHandlerLock);
    
    if (!iihdata->InputDeviceTask) iihdata->InputDeviceTask = FindTask(NULL);
    
    /* First handle IntuiMessages which were replied back to the IntuiReplyPort
       by the apps */
       
    HandleIntuiReplyPort(iihdata, IntuitionBase);
    
    /* Then free generated InputEvents done in the previous round */
    
    FreeGeneratedInputEvents(iihdata);
    
    /* Now handle the input events */
    
    ie = &stackie;
    orig_ie = iihdata->ActInputEvent = oldchain;
    if (orig_ie) *ie = *orig_ie;
    iihdata->ActInputEventUsed = FALSE;
    
    while(orig_ie)
    {
    
	struct Window 	*old_w;
	BOOL 	    	swallow_event = FALSE;
	BOOL 	    	new_active_window = FALSE;
    
    	D(bug("iih: Handling event of class %d, code %d\n", ie->ie_Class, ie->ie_Code));
	reuse_event = FALSE;
	
	/* Use event to find the active window */
        
	w = IntuitionBase->ActiveWindow;
	
	if (!MENUS_ACTIVE && !SYSGADGET_ACTIVE)
	{
            /* lock = LockIBase(0UL); */

    	    old_w = IntuitionBase->ActiveWindow;
	    if (ie->ie_Class == IECLASS_RAWMOUSE && ie->ie_Code == SELECTDOWN)
	    {
		w = FindActiveWindow(ie, &swallow_event, IntuitionBase);
	    }

	    D(bug("iih:New active window: %p\n", w));


	    if ( w != old_w )
	    {
		if (w)
		{
	            D(bug("Activating new window (title %s)\n", w->Title ? w->Title : "<noname>"));

	            D(bug("Window activated\n"));
		}
		else
		{
	    	    D(bug("Making active window inactive. Now there's no active window\n"));
		}
		new_active_window = TRUE;
	    }

 	    /* UnlockIBase(lock); */

	    if (new_active_window)
	    {
		/* If there was an active gadget in the old window
		   we must make it inactive */

		if (gadget)
		{
	    	    switch (gadget->GadgetType & GTYP_GTYPEMASK)
		    {

			case GTYP_CUSTOMGADGET:
			    {
		    		struct gpGoInactive gpgi;

				gpgi.MethodID = GM_GOINACTIVE;
				gpgi.gpgi_GInfo = gi;
				gpgi.gpgi_Abort = 1; 

				Locked_DoMethodA((Object *)gadget, (Msg)&gpgi, IntuitionBase);
				
				/* No need to handle iihdata->ActiveSysGadget here, as if
				   there's an active sys gadget we cannot get here, because
				   of the "if (!MENUS_ACTIVE && !SYSGADGET_ACTIVE)" check
				   above */
			    }
			    break;

			case GTYP_STRGADGET:
			    gadget->Flags &= ~GFLG_SELECTED;
			    RefreshStrGadget(gadget, gi->gi_Window, IntuitionBase);
			    break;

			case GTYP_BOOLGADGET:
			    /* That a bool gadget is active here can only happen
			       if user used LMB to activate gadget and LAMIGA + LALT
			       to activate other window, or viceversa */

    	    	    	    if (!(gadget->Activation & GACT_TOGGLESELECT))
			    {
    	    	    	    	BOOL inside;
				
				inside = InsideGadget(gi->gi_Screen, gi->gi_Window, gadget, gi->gi_Screen->MouseX, gi->gi_Screen->MouseY);
				
				if (inside)
				{
				    gadget->Flags ^= GFLG_SELECTED;
			    	    RefreshBoolGadgetState(gadget, gi->gi_Window, IntuitionBase);
				}
			    }
			    break;

	        	case GTYP_PROPGADGET:
			    /* That a prop gadget is active here can only happen
			       if user used LMB to activate gadget and LAMIGA + LALT
			       to activate other window, or viceversa */

			    HandlePropSelectUp(gadget, gi->gi_Window, NULL, IntuitionBase);
			    if (gadget->Activation & GACT_RELVERIFY)
			    {
				ih_fire_intuimessage(gi->gi_Window,
			    			     IDCMP_GADGETUP,
						     0,
						     gadget,
						     IntuitionBase);
			    }
			    break;

		    } /* switch (gadget->GadgetType & GTYP_GTYPEMASK) */

		    gadget->Activation &= ~GACT_ACTIVEGADGET;
		    iihdata->ActiveGadget = NULL;
		    gadget = NULL;

		} /* if (gadget) */

		if(old_w)
		{
		    ih_fire_intuimessage(old_w,
					 IDCMP_INACTIVEWINDOW,
					 0,
					 old_w,
					 IntuitionBase);
		}

		/* int_activatewindow works if w = NULL */
		int_activatewindow(w, IntuitionBase);

		if (w)
		{
		    ih_fire_intuimessage(w,
					 IDCMP_ACTIVEWINDOW,
					 0,
					 w,
					 IntuitionBase);
		}


	    } /* if (new_active_window) */

	} /* if (!MENUS_ACTIVE) */
	
	if (!swallow_event) switch (ie->ie_Class)
	{	    
	    case IECLASS_RAWMOUSE:
		switch (ie->ie_Code)
		{
		    case SELECTDOWN:
		    {
			BOOL new_gadget = FALSE;

			iihdata->ActQualifier |= IEQUALIFIER_LEFTBUTTON;

			if (MENUS_ACTIVE)
			{
			    FireMenuMessage(MMCODE_EVENT, 0, ie, IntuitionBase);
			    orig_ie->ie_Class = IECLASS_NULL;
			    break;
			}

			if (!gadget && IntuitionBase->ActiveScreen)
			{
  			    gadget = FindGadget (IntuitionBase->ActiveScreen,
		    				 w,
						 IntuitionBase->ActiveScreen->MouseX,
						 IntuitionBase->ActiveScreen->MouseY, gi, IntuitionBase);
			    if (gadget)
			    {
			    	BOOL is_draggad, is_sizegad;
				
		        	/* Whenever the active gadget changes the gi must be updated
				   because it is cached in iidata->GadgetInfo!!!! Don't
				   forget to do this if somewhere else the active
				   gadget is changed, for example in ActivateGadget!!! */

		        	PrepareGadgetInfo(gi, IntuitionBase->ActiveScreen, w);
		    		SetGadgetInfoGadget(gi, gadget);

   	    	    	    	is_draggad = ((gadget->GadgetType & GTYP_SYSTYPEMASK) == GTYP_WDRAGGING);
				is_sizegad = ((gadget->GadgetType & GTYP_SYSTYPEMASK) == GTYP_SIZING);

    	    	    	    	if (is_draggad || is_sizegad)
				{
				    if (IS_BOOPSI_GADGET(gadget))
				    {
					DoGPInput(gi,
						  gadget,
						  ie,
						  GM_GOACTIVE,
						  &reuse_event,
						  IntuitionBase);
						  
					/* Ignoring retval of dispatcher above is what AmigaOS does too for
					   boopsi drag/resize gadgets */
					   
				    }
				    
				    /* From now on the master drag/size gadget takes over */
				    
				    iihdata->ActiveSysGadget = gadget;
				    gadget = is_draggad ? iihdata->MasterDragGadget : iihdata->MasterSizeGadget;
				}
				else if (gadget->Activation & GACT_IMMEDIATE)
				{
				    ih_fire_intuimessage(w,
					      		 IDCMP_GADGETDOWN,
					      		 0,
					      		 gadget,
					      		 IntuitionBase);
				}
 
				new_gadget = TRUE;
			    }

			} /* if (!gadget) */

			if (gadget)
			{			    
			    switch (gadget->GadgetType & GTYP_GTYPEMASK)
			    {
				case GTYP_BOOLGADGET:
				    //if (gadget->Activation & GACT_TOGGLESELECT)
				    //{
					gadget->Flags ^= GFLG_SELECTED;
					RefreshBoolGadgetState(gadget, w, IntuitionBase);
				    //}
				    //else if (!(gadget->Flags & GFLG_SELECTED))
				    //{
				    //	gadget->Flags |= GFLG_SELECTED;
				    //	RefreshBoolGadgetState(gadget, w, IntuitionBase);			    
				    //}
				    break;

				case GTYP_PROPGADGET:
				    HandlePropSelectDown(gadget,
							 w,
							 NULL,
							 w->MouseX - gi->gi_Domain.Left - GetGadgetLeft(gadget, gi->gi_Screen, gi->gi_Window, NULL),
							 w->MouseY - gi->gi_Domain.Top  - GetGadgetTop(gadget, gi->gi_Screen, gi->gi_Window, NULL),
							 IntuitionBase);


				    break;

				case GTYP_STRGADGET:
				    /* If the click was inside the active strgad,
				    ** then let it update cursor pos,
				    ** else deactivate stringadget and reuse event.
				    */

				    if (InsideGadget(gi->gi_Screen, gi->gi_Window, gadget, gi->gi_Screen->MouseX, gi->gi_Screen->MouseY))
				    {
					UWORD imsgcode;

					HandleStrInput(gadget, gi, ie, &imsgcode,
						       IntuitionBase);
				    }
				    else
				    {
					gadget->Flags &= ~GFLG_SELECTED;

					RefreshStrGadget(gadget, w, IntuitionBase);
					/* Gadget not active anymore */
					gadget = NULL;
					reuse_event = TRUE;
				    }
				    break;

				case GTYP_CUSTOMGADGET:
				    gadget = DoGPInput(gi,
						       gadget,
						       ie,
						       (new_gadget ? GM_GOACTIVE : GM_HANDLEINPUT),
						       &reuse_event,
						       IntuitionBase);

				    break;


			    } /* switch (GadgetType) */

			} /* if (a gadget is active) */
			else if (w)
			{
			    ih_fire_intuimessage(w,
		    		      		 IDCMP_MOUSEBUTTONS,
				      		 SELECTDOWN,
				      		 w,
				      		 IntuitionBase);
			}

			break;

		    } /* case SELECTDOWN */

		    case SELECTUP:
			iihdata->ActQualifier &= ~IEQUALIFIER_LEFTBUTTON;

			if (MENUS_ACTIVE)
			{
			    FireMenuMessage(MMCODE_EVENT, 0, ie, IntuitionBase);
			    orig_ie->ie_Class = IECLASS_NULL;
			    break;
			}

			if (gadget)
			{
			    BOOL inside = InsideGadget(gi->gi_Screen, gi->gi_Window, gadget, gi->gi_Screen->MouseX, gi->gi_Screen->MouseY);

			    /*int selected = (gadget->Flags & GFLG_SELECTED) != 0;*/


			    switch (gadget->GadgetType & GTYP_GTYPEMASK)
			    {
				case GTYP_BOOLGADGET:
				    if (!(gadget->Activation & GACT_TOGGLESELECT))
				    {
				    	if (inside)
					{
			    		    gadget->Flags ^= GFLG_SELECTED;
					    RefreshBoolGadgetState(gadget, w, IntuitionBase);
					}
   	    	    		    }
				    else
				    {
					inside = TRUE;
				    }

				    if (inside && (gadget->Activation & GACT_RELVERIFY))
				    {
			    		if (IS_SYS_GADGET(gadget))
					{
					    HandleSysGadgetVerify(gi, gadget, IntuitionBase);
					}
					else
					{
					    ih_fire_intuimessage(w,
			    		      			 IDCMP_GADGETUP,
					      			 0,
					      			 gadget,
					      			 IntuitionBase);
					}
				    }
				    else
				    {
					/* RKRM say so */
					ih_fire_intuimessage(w,
			    		      		     IDCMP_MOUSEBUTTONS,
					      		     SELECTUP,
					      		     w,
					      		     IntuitionBase);
				    }

				    gadget = NULL;
				    break;

				case GTYP_PROPGADGET:
				    HandlePropSelectUp(gadget, w, NULL, IntuitionBase);
				    if (gadget->Activation & GACT_RELVERIFY)
				    {
					ih_fire_intuimessage(w,
			    		      		     IDCMP_GADGETUP,
					      		     0,
					      		     gadget,
					      		     IntuitionBase);
				    }


				    gadget = NULL;
				    break;

				/* Intuition string gadgets don't care about SELECTUP */

				case GTYP_CUSTOMGADGET:
				    gadget = DoGPInput(gi, gadget, ie, GM_HANDLEINPUT, &reuse_event, IntuitionBase);
				    break;

			    } /* switch GadgetType */

			} /* if (a gadget is currently active) */
			else if (w)
			{
			    ih_fire_intuimessage(w,
		    		      		 IDCMP_MOUSEBUTTONS,
				      		 SELECTUP,
				      		 w,
				      		 IntuitionBase);
			}

			break; /* case SELECTUP */

		    case MENUDOWN:
	        	iihdata->ActQualifier |= IEQUALIFIER_RBUTTON;

			if (MENUS_ACTIVE)
			{
			    FireMenuMessage(MMCODE_EVENT, 0, ie, IntuitionBase);
			    orig_ie->ie_Class = IECLASS_NULL;
			    break;
			}

	        	if (w && !gadget)
			{
			    if (!(w->Flags & WFLG_RMBTRAP))
			    {
				if (FireMenuMessage(MMCODE_START, w, ie, IntuitionBase))
				{
				    /* This lock will be released only when the user is
				       done with menus = when IECLASS_MENU + IESUBCLASS_MENUSTOP
				       event arrives (generated by MenuHandler task) */

				    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->MenuLock);
				    iihdata->MenuWindow = w;
				    MENUS_ACTIVE = TRUE;
				}			
			    }
			}
			/* fall through */

		    case MENUUP:
		    case MIDDLEDOWN:
		    case MIDDLEUP:
	    		switch(ie->ie_Code)
			{
			    case MENUUP:
		    		iihdata->ActQualifier &= ~IEQUALIFIER_RBUTTON;
				break;

			    case MIDDLEDOWN:
		    		iihdata->ActQualifier |= IEQUALIFIER_MIDBUTTON;
				break;

			    case MIDDLEUP:
		    		iihdata->ActQualifier &= ~IEQUALIFIER_MIDBUTTON;
				break;
			}

			if (MENUS_ACTIVE)
			{
			    FireMenuMessage(MMCODE_EVENT, 0, ie, IntuitionBase);
			    orig_ie->ie_Class = IECLASS_NULL;
			    break;
			}

			if (gadget)
			{
			    if (IS_BOOPSI_GADGET(gadget))
			    {
				gadget = DoGPInput(gi, gadget, ie, GM_HANDLEINPUT, &reuse_event, IntuitionBase);
			    }

			} /* if (there is an active gadget) */
			else if (w)
			{
			    ih_fire_intuimessage(w,
		    		      		 IDCMP_MOUSEBUTTONS,
				      		 ie->ie_Code,
				      		 w,
				      		 IntuitionBase);
			}

			break; /* case MENUDOWN */

		    case IECODE_NOBUTTON: /* MOUSEMOVE */
		    {
	    		if (MouseCoordsRelative())
			{
			    struct Screen *scr;

			    iihdata->DeltaMouseX = ie->ie_X;
			    iihdata->DeltaMouseY = ie->ie_Y;

			    ie->ie_X = iihdata->DeltaMouseX + iihdata->LastMouseX;
			    ie->ie_Y = iihdata->DeltaMouseY + iihdata->LastMouseY;

			    if (ie->ie_X < 0) ie->ie_X = 0;
			    if (ie->ie_Y < 0) ie->ie_Y = 0;

			    lock = LockIBase(0);
			    scr = IntuitionBase->ActiveScreen;

			    if (scr)
			    {
		    		if (ie->ie_X >= scr->Width)  ie->ie_X = scr->Width - 1;
				if (ie->ie_Y >= scr->Height) ie->ie_Y = scr->Height - 1;
			    }
			    else
			    {
		    		if (ie->ie_X >= 320) ie->ie_X = 320 - 1;
				if (ie->ie_Y >= 200) ie->ie_Y = 200 - 1;
			    }
   			    UnlockIBase(lock);

			}
			else
			{
			    iihdata->DeltaMouseX = ie->ie_X - iihdata->LastMouseX;
			    iihdata->DeltaMouseY = ie->ie_Y - iihdata->LastMouseY;
			}

    	    	    #if SINGLE_SETPOINTERPOS_PER_EVENTLOOP
			SetPointerPos(ie->ie_X, ie->ie_Y);
    	    	    #else
		    	pointerposx = ie->ie_X;
			pointerposy = ie->ie_Y;
			call_setpointerpos = TRUE;
		    #endif
		    
			iihdata->LastMouseX = ie->ie_X;
			iihdata->LastMouseY = ie->ie_Y;

			notify_mousemove_screensandwindows(ie->ie_X, 
		                                	   ie->ie_Y, 
		                                	   IntuitionBase);

			if (MENUS_ACTIVE)
			{
			    FireMenuMessage(MMCODE_EVENT, 0, ie, IntuitionBase);
			    orig_ie->ie_Class = IECLASS_NULL;
			    break;
			}

			if (gadget)
			{			    
			    orig_ie->ie_Class = IECLASS_NULL;

			    switch (gadget->GadgetType & GTYP_GTYPEMASK)
			    {
				case GTYP_BOOLGADGET:
		    		    if (!(gadget->Activation & GACT_TOGGLESELECT))
				    {
					BOOL inside, oldinside;

					inside = InsideGadget(gi->gi_Screen,
			    	    	    		      gi->gi_Window,
							      gadget,
							      gi->gi_Screen->MouseX,
							      gi->gi_Screen->MouseY);

					oldinside = InsideGadget(gi->gi_Screen,
			    	    	    			 gi->gi_Window,
								 gadget,
								 gi->gi_Screen->MouseX - iihdata->DeltaMouseX,
								 gi->gi_Screen->MouseY - iihdata->DeltaMouseY);
								 
					if  (inside != oldinside)
					{
					    gadget->Flags ^= GFLG_SELECTED;
					    RefreshBoolGadgetState(gadget, w, IntuitionBase);
					}
				    }
				    break;
 				
				case GTYP_PROPGADGET:
				    HandlePropMouseMove(gadget
					    ,w
					    ,NULL
					    ,w->MouseX - gi->gi_Domain.Left - GetGadgetLeft(gadget, gi->gi_Screen, gi->gi_Window, NULL)
					    ,w->MouseY - gi->gi_Domain.Top  - GetGadgetTop(gadget, gi->gi_Screen, gi->gi_Window, NULL)
					    ,IntuitionBase);

				    break;

				case GTYP_CUSTOMGADGET:
				    gadget = DoGPInput(gi, gadget, ie, GM_HANDLEINPUT, &reuse_event, IntuitionBase);
				    break;

			    } /* switch GadgetType */

			} /* if (a gadget is currently active) */

			orig_ie->ie_Class = IECLASS_NULL;

			if (!w) break;

			if (!(w->IDCMPFlags & IDCMP_MOUSEMOVE)) break;

			/* Send IDCMP_MOUSEMOVE if WFLG_REPORTMOUSE is set
			   and/or active gadget has GACT_FOLLOWMOUSE set */

			if (!(w->Flags & WFLG_REPORTMOUSE))
			{
			    if (!gadget) break;
			    if (!(gadget->Activation & GACT_FOLLOWMOUSE)) break;
			}

			orig_ie->ie_Class = IECLASS_RAWMOUSE;

			/* Limit the number of IDCMP_MOUSEMOVE messages sent to intuition.
			   note that this comes after handling gadgets, because gadgets should get all events.
			*/

			if (IW(w)->num_mouseevents >= IW(w)->mousequeue)
			{
			    BOOL old_msg_found = FALSE;

			    /* Mouse Queue is full, so try looking for a not
			       yet GetMsg()ed IntuiMessage in w->UserPort
			       trying to modify that. */

			    Forbid();   
			    if (w->UserPort)
			    {
		        	struct IntuiMessage *im;

				for (im = (struct IntuiMessage *)w->UserPort->mp_MsgList.lh_TailPred;
				     im->ExecMessage.mn_Node.ln_Pred;
				     im = (struct IntuiMessage *)im->ExecMessage.mn_Node.ln_Pred)			
				{
				    if ((im->Class == IDCMP_MOUSEMOVE) &&
			        	(im->IDCMPWindow == w))
				    {
			        	im->Qualifier = iihdata->ActQualifier;

			        	if (w->IDCMPFlags & IDCMP_DELTAMOVE)
					{
					    im->MouseX = iihdata->DeltaMouseX;
					    im->MouseY = iihdata->DeltaMouseY;
					}
					else
					{
					    im->MouseX = w->MouseX;
					    im->MouseY = w->MouseY;
					}   
			        	CurrentTime(&im->Seconds, &im->Micros);

			        	old_msg_found = TRUE;
					break;
				    }
				}
			    } /* if (w->UserPort) */
			    Permit();

			    /* no need to send a new message if we modified
			       an existing one ... */

			    if (old_msg_found) break;

			    /* ... otherwise we are in a strange situation. The mouse
			       queue is full, but we did not find an existing MOUSEMOVE
			       imsg in w->UserPort. So the app probably has removed
			       an imsg from the UserPort with GetMsg but we did not get
			       the ReplyMsg, yet. In this case we do send a new message */

			    HandleIntuiReplyPort(iihdata, IntuitionBase);

			}

			/* MouseQueue is not full, so we can send a message. We increase
			   IntWindow->num_mouseevents which will later be decreased after
			   the Intuition InputHandler gets the ReplyMessage from the app
			   and handles it in HandleIntuiReplyPort() */

			if (ih_fire_intuimessage(w,
				      		 IDCMP_MOUSEMOVE,
				      		 IECODE_NOBUTTON,
				      		 w,
				      		 IntuitionBase))
			{
			    IW(w)->num_mouseevents++;
			}

	        	break;
			
		    } /* case IECODE_NOBUTTON */

		} /* switch (ie->ie_Code)  (what button was pressed ?) */
		break;

    #define KEY_QUALIFIERS (IEQUALIFIER_LSHIFT     | IEQUALIFIER_RSHIFT   | \
			    IEQUALIFIER_CAPSLOCK   | IEQUALIFIER_CONTROL  | \
			    IEQUALIFIER_LALT       | IEQUALIFIER_RALT     | \
			    IEQUALIFIER_LCOMMAND   | IEQUALIFIER_RCOMMAND | \
			    IEQUALIFIER_NUMERICPAD | IEQUALIFIER_REPEAT)

	    case IECLASS_RAWKEY:
		/* release events go only to gadgets and windows who
		   have not set IDCMP_VANILLAKEY */


		iihdata->ActQualifier &= ~KEY_QUALIFIERS;
		iihdata->ActQualifier |= (ie->ie_Qualifier & KEY_QUALIFIERS);

    	    	/* Keyboard mouse emulation */

		{
		    UWORD code = ie->ie_Code & ~IECODE_UP_PREFIX;
		    
		    /* Mouse button emulation: LALT + LAMIGA = LBUTTON, RALT + RAMIGA = RBUTTON */
		    
		    if ((code == RAWKEY_LAMIGA) ||
		        (code == RAWKEY_LALT)   ||
			(code == RAWKEY_RAMIGA) ||
			(code == RAWKEY_RALT))
		    {
    	    		 iihdata->PrevKeyMouseState = iihdata->ActKeyMouseState;
			 iihdata->ActKeyMouseState = 0;
    	    		 if ((ie->ie_Qualifier & (IEQUALIFIER_LCOMMAND | IEQUALIFIER_LALT)) == (IEQUALIFIER_LCOMMAND | IEQUALIFIER_LALT))
			 {
			     iihdata->ActKeyMouseState |= IEQUALIFIER_LEFTBUTTON;
			 }
    	    		 if ((ie->ie_Qualifier & (IEQUALIFIER_RCOMMAND | IEQUALIFIER_RALT)) == (IEQUALIFIER_RCOMMAND | IEQUALIFIER_RALT))
			 {
			     iihdata->ActKeyMouseState |= IEQUALIFIER_RBUTTON;
			 }

			 if ((iihdata->ActKeyMouseState & IEQUALIFIER_LEFTBUTTON) != (iihdata->PrevKeyMouseState & IEQUALIFIER_LEFTBUTTON))
			 {
			      orig_ie->ie_Class    = IECLASS_RAWMOUSE;
			      orig_ie->ie_SubClass = 0;
			      orig_ie->ie_Code     = (iihdata->ActKeyMouseState & IEQUALIFIER_LEFTBUTTON) ? IECODE_LBUTTON : IECODE_LBUTTON | IECODE_UP_PREFIX;
			      orig_ie->ie_X        = 0;
			      orig_ie->ie_Y        = 0;
			      *ie = *orig_ie;

			      reuse_event = TRUE;
			      break;
			 }

			 if ((iihdata->ActKeyMouseState & IEQUALIFIER_RBUTTON) != (iihdata->PrevKeyMouseState & IEQUALIFIER_RBUTTON))
			 {
			      orig_ie->ie_Class    = IECLASS_RAWMOUSE;
			      orig_ie->ie_SubClass = 0;
			      orig_ie->ie_Code     = (iihdata->ActKeyMouseState & IEQUALIFIER_RBUTTON) ? IECODE_RBUTTON : IECODE_RBUTTON | IECODE_UP_PREFIX;
			      orig_ie->ie_X        = 0;
			      orig_ie->ie_Y        = 0;
			      *ie = *orig_ie;

			      reuse_event = TRUE;
			      break;
			 }
			 
		    } /* if key is one of LAMIGA/LALT/RAMIGA/RALT */ 
		    
		    if ((iihdata->ActQualifier & (IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND)) &&
		    	MouseCoordsRelative() &&
    	    	    	((ie->ie_Code == RAWKEY_UP)    ||
		    	 (ie->ie_Code == RAWKEY_DOWN)  ||
			 (ie->ie_Code == RAWKEY_LEFT)  ||
			 (ie->ie_Code == RAWKEY_RIGHT)))
		    {
		    	static BYTE xmap[] = { 0, 0, 1, -1};
			static BYTE ymap[] = {-1, 1, 0,  0};
			WORD        shift;
			
			shift = (iihdata->ActQualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) ? 40 : 1;
			
		      	/* Mouse Move Emulation */
			
			orig_ie->ie_Class    = IECLASS_RAWMOUSE;
			orig_ie->ie_SubClass = 0;
			orig_ie->ie_Code     = IECODE_NOBUTTON;
			orig_ie->ie_X	     = xmap[code - RAWKEY_UP] * shift;
			orig_ie->ie_Y	     = ymap[code - RAWKEY_UP] * shift;
			
			*ie = *orig_ie;
			reuse_event = TRUE;
			break;
		    }
		    
		} /**/
		
    	    	/* End Keyboard mouse emulation */
		
		if (MENUS_ACTIVE)
		{
		    FireMenuMessage(MMCODE_EVENT, 0, ie, IntuitionBase);
		    orig_ie->ie_Class = IECLASS_NULL;
		    break;
		}

		if ( (!(ie->ie_Code & IECODE_UP_PREFIX)) ||
	             gadget ||
	             (w && ((w->IDCMPFlags & IDCMP_VANILLAKEY) == 0)) )
		{

		    if (gadget)
		    {
			orig_ie->ie_Class = IECLASS_NULL;

			switch (gadget->GadgetType & GTYP_GTYPEMASK)
			{
			    case GTYP_STRGADGET:
			    {
				UWORD imsgcode;
				ULONG ret = HandleStrInput(gadget, gi, ie, &imsgcode,
							   IntuitionBase);

				if (ret & (SGA_END | SGA_NEXTACTIVE | SGA_PREVACTIVE))
				{
				    if (gadget->Activation & GACT_RELVERIFY)
				    {
			        	ih_fire_intuimessage(w,
							     IDCMP_GADGETUP,
							     imsgcode,
							     gadget,
							     IntuitionBase);

				    }

				    if ((gadget->Flags & GFLG_TABCYCLE) && (ret & SGA_NEXTACTIVE))
				    {
					gadget = FindCycleGadget(w, gadget, GMR_NEXTACTIVE);
				    }
				    else if ((gadget->Flags & GFLG_TABCYCLE) && (ret & SGA_PREVACTIVE))
				    {
					gadget = FindCycleGadget(w, gadget, GMR_PREVACTIVE);
				    }
				    else
				    {
					gadget = NULL;
				    }

				    if (gadget)
				    {
					gadget = DoActivateGadget(w, gadget, IntuitionBase);
				    }

				} /* if (ret & (SGA_END | SGA_NEXTACTIVE | SGA_PREVACTIVE)) */

				break;
			    }

			    case GTYP_CUSTOMGADGET:
				gadget = DoGPInput(gi, gadget, ie, GM_HANDLEINPUT, &reuse_event, IntuitionBase);
				break;

			} /* switch (gadget type) */

		    } /* if (a gadget is currently active) */
		    else if (w)
		    {
			BOOL menushortcut = FALSE;

			if ((ie->ie_Qualifier & IEQUALIFIER_RCOMMAND) &&
		            (!(w->Flags & WFLG_RMBTRAP)) &&
			    (w->IDCMPFlags & IDCMP_MENUPICK))
			{
			    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->MenuLock);

			    if (w->MenuStrip)
			    {
				UBYTE key;

		        	if (MapRawKey(ie, &key, 1, NULL) == 1)
				{
				    UWORD menucode;

				    menucode = FindMenuShortCut(w->MenuStrip, key, TRUE, IntuitionBase);
				    if (menucode != MENUNULL)
				    {
					ie->ie_Class        = IECLASS_MENU;
					ie->ie_SubClass     = IESUBCLASS_MENUSTOP;
					ie->ie_EventAddress = w;
					ie->ie_Code         = menucode;

					reuse_event = TRUE;
					menushortcut = TRUE;

					MENUS_ACTIVE = TRUE;
    	    	    	    		iihdata->MenuWindow = w;
				    }
				}
			    }
			    if (!menushortcut) /* !! */
				ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->MenuLock);

			} /* if could be a menu short cut */

			if (menushortcut) break;

			/* This is a regular RAWKEY event (no gadget taking care
			   of it...). */

			if (iihdata->ActQualifier & IEQUALIFIER_REPEAT)
			{			
		            /* don't send repeat key events if repeatqueue is full */			
		    	    if (IW(w)->num_repeatevents >= IW(w)->repeatqueue) break;
			}

			if (w->IDCMPFlags & IDCMP_VANILLAKEY)
			{
			    UBYTE keyBuffer;

			    if (MapRawKey(ie, &keyBuffer, 1, NULL) == 1)
			    {
				ih_fire_intuimessage(w,
			    		      	     IDCMP_VANILLAKEY,
					      	     keyBuffer,
					      	     ie->ie_position.ie_addr, /* ie_dead.ie_prev[1|2]Down[Code|Qual]. 64 bit machines!? */
					      	     IntuitionBase);
				break;
			    } 

			    /* If the event mapped to more than one byte, it is not
			       a legal VANILLAKEY, so we send it as the original
			       RAWKEY event. */

			}

			ih_fire_intuimessage(w,
		    		      	     IDCMP_RAWKEY,
				      	     ie->ie_Code,
				      	     ie->ie_position.ie_addr, /* ie_dead.ie_prev[1|2]Down[Code|Qual]. 64 bit machine!? */
				      	     IntuitionBase);

		    } /* regular RAWKEY */
		}
		break; /* case IECLASS_RAWKEY */

	    case IECLASS_TIMER:	    	    
		if (MENUS_ACTIVE)
		{
		    FireMenuMessage(MMCODE_EVENT, 0, ie, IntuitionBase);
		    orig_ie->ie_Class = IECLASS_NULL;
		    break;
		}

		if (gadget)
		{
	            if (IS_BOOPSI_GADGET(gadget))
		    {
			gadget = DoGPInput(gi, gadget, ie, GM_HANDLEINPUT, &reuse_event, IntuitionBase);
		    }

		} /* if (gadget) */

		/* stegerg: on the Amiga, Intuition's InputHandler seems to always
		   swallow IECLASS_TIMER InputEvents. They never reach InputHandlers with
		   lower priorities. So we mark the event as eaten by Intuition */	       
		orig_ie->ie_Class = IECLASS_NULL;

		if (!w) break;

		/* Send INTUITICK msg only if app already replied the last INTUITICK msg */
		if (w->Flags & WFLG_WINDOWTICKED) break;

		/* Set the WINDOWTICKED flag, it will be cleared again when the app
		   replies back the msg and the InputHandler handles the replymsg
		   in HandleIntuiReplyPort() */

		ih_fire_intuimessage(w,
	    		      	     IDCMP_INTUITICKS,
			      	     0,
			      	     w,
			      	     IntuitionBase);

		break; /* case IECLASS_TIMER */

	    case IECLASS_MENU:
		if (MENUS_ACTIVE && (ie->ie_SubClass == IESUBCLASS_MENUSTOP))
		{
	    	    iihdata->MenuWindow = NULL;
	            MENUS_ACTIVE = FALSE;

		    /* semaphore was locked when menu action started, see
		       above where MMCODE_START MenuMessage is sent.

		       It could have also have been looked if the user
		       activated one of the menu key shortcuts, see
		       "case IECLASS_RAWKEY" */

    		    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->MenuLock);

	            orig_ie->ie_Class = IECLASS_NULL;

		    ih_fire_intuimessage((struct Window *)ie->ie_EventAddress,
		    			 IDCMP_MENUPICK,
					 ie->ie_Code,
					 (struct Window *)ie->ie_EventAddress,
					 IntuitionBase);

		}
		break;

	    default:
		if (MENUS_ACTIVE)
		{
		    FireMenuMessage(MMCODE_EVENT, 0, ie, IntuitionBase);
		    orig_ie->ie_Class = IECLASS_NULL;
		    break;
		}

    	    	if (ie->ie_Class != IECLASS_NULL)
		{
        	    bug("[Intui] InputHandler: Unknown IEClass: addr = %x  class = %d (origclass = %d)\n",orig_ie, ie->ie_Class,orig_ie->ie_Class);
		}
		break;
	    
	} /* switch (ie->ie_Class) */

	if (!reuse_event)
	{
	    orig_ie = iihdata->ActInputEvent = orig_ie->ie_NextEvent;
	    if (orig_ie) *ie = *orig_ie;
	    iihdata->ActInputEventUsed = FALSE;
	}
	
    } /* for (each event in the chain) */

    iihdata->ActiveGadget = gadget;
    iihdata->ActInputEvent = 0;

    D(bug("Outside pollingloop\n"));

#if SINGLE_SETPOINTERPOS_PER_EVENTLOOP
    if (call_setpointerpos)
    {
    	SetPointerPos(pointerposx, pointerposy);
    }
#endif
    
    {
        struct InputEvent *last_ie = NULL;

        /* Remove eaten InputEvents */
	
	iihdata->ReturnInputEvent = 0;
	for(ie = oldchain; ie; ie = ie->ie_NextEvent)
	{
            if (ie->ie_Class != IECLASS_NULL)
	    {
		if (last_ie)
		{
	            last_ie->ie_NextEvent = ie;
		}
		else
		{
	            iihdata->ReturnInputEvent = ie;
		}
		last_ie = ie;
	    }
	}

        HandleIntuiActions(iihdata, IntuitionBase);
    
   	/* Add generated InputEvents */
	
	if (last_ie)
	{
	    last_ie->ie_NextEvent = iihdata->GeneratedInputEvents;
	}
	else
	{
	    iihdata->ReturnInputEvent = iihdata->GeneratedInputEvents;
	}
    }
        
    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->InputHandlerLock);

    return iihdata->ReturnInputEvent;

    AROS_USERFUNC_EXIT
}

/****************************************************************************************/

