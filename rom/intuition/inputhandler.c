#define AROS_ALMOST_COMPATIBLE 1 /* NEWLIST macro */
#include <proto/exec.h>
#include <proto/boopsi.h>
#include <proto/intuition.h>
#include <proto/alib.h>
#include <proto/layers.h>
#include <proto/graphics.h>
#include <proto/keymap.h>
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
#include "inputhandler.h"

#include "boopsigadgets.h"
#include "boolgadgets.h"
#include "propgadgets.h"
#include "strgadgets.h"
#include "gadgets.h"
#include "intuition_intern.h" /* EWFLG_xxx */
#include "inputhandler_support.h"
#include "inputhandler_actions.h"

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

#define IW(x) ((struct IntWindow *)(x))


/***************
**  InitIIH   **
***************/

struct Interrupt *InitIIH(struct IntuitionBase *IntuitionBase)
{
    struct Interrupt *iihandler;
    struct IIHData *iihdata;

    D(bug("InitIIH(IntuitionBase=%p)\n", IntuitionBase));

    iihandler = AllocMem(sizeof (struct Interrupt), MEMF_PUBLIC|MEMF_CLEAR);
    if (iihandler)
    {
	iihdata = AllocMem(sizeof (struct IIHData), MEMF_PUBLIC|MEMF_CLEAR);
	if (iihdata)
	{
	    struct MsgPort *port;
	    
	    /* Multiply with 2 because we need memory for two MsgPorts!!! */
	    port = AllocMem(sizeof (struct MsgPort) * 2, MEMF_PUBLIC|MEMF_CLEAR);
	    if (port)
	    {
	        ULONG lock;
		
		/* We do not want to be woken up by message replies.
		   We are anyway woken up about 10 times a second by
		   timer events
		*/
	    	port->mp_Flags   = PA_IGNORE;
		
	    	/* stegerg: PA_IGNORE doesn't need mp_SigBit and mp_SigTask
		port->mp_SigBit  = SIGB_INTUITION;
	    	port->mp_SigTask = FindTask("input.device");
		*/
	    	NEWLIST( &(port->mp_MsgList) );
	    	iihdata->IntuiReplyPort = port;
	    	
		port++;
		
		port->mp_Flags	 = PA_IGNORE;
		NEWLIST( &(port->mp_MsgList) );
		iihdata->IntuiDeferedActionPort = port;
		
		iihandler->is_Code = (APTR)AROS_ASMSYMNAME(IntuiInputHandler);
		iihandler->is_Data = iihdata;
		iihandler->is_Node.ln_Pri	= 50;
		iihandler->is_Node.ln_Name	= "Intuition InputHandler";
		
		lock = LockIBase(0UL);

		iihdata->IntuitionBase = IntuitionBase;
		
		UnlockIBase(lock);
		
		GetPrivIBase(IntuitionBase)->IntuiReplyPort = iihdata->IntuiReplyPort;
		GetPrivIBase(IntuitionBase)->IntuiDeferedActionPort = iihdata->IntuiDeferedActionPort;

		ReturnPtr ("InitIIH", struct Interrupt *, iihandler);
	    }
	    FreeMem(iihdata, sizeof (struct IIHData));
	}
	FreeMem(iihandler, sizeof (struct Interrupt));
    }
    ReturnPtr ("InitIIH", struct Interrupt *, NULL);
}

/****************
** CleanupIIH  **
****************/

VOID CleanupIIH(struct Interrupt *iihandler, struct IntuitionBase *IntuitionBase)
{
    /* One might think that this port is still in use by the inputhandler.
    ** However, if intuition is closed for the last time, there should be no
    ** windows that IntuiMessage can be sent to.
    **
    ** sizeof(struct MsgPort) * 2, because memory for two MsgPorts was allocated!
    */
    FreeMem(((struct IIHData *)iihandler->is_Data)->IntuiReplyPort, sizeof (struct MsgPort) * 2);
    
    
    FreeMem(iihandler->is_Data, sizeof (struct IIHData));
    FreeMem(iihandler, sizeof (struct Interrupt));

    return;
}



/***************************
**  HandleIntuiReplyPort  **
***************************/
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
		
	} /* switch(im->Class) */
    	free_intuimessage(im, IntuitionBase);
	
    } /* while ((im = (struct IntuiMessage *)GetMsg(iihdata->IntuiReplyPort))) */
}

/************************
**  IntuiInputHandler  **
************************/
AROS_UFH2(struct InputEvent *, IntuiInputHandler,
    AROS_UFHA(struct InputEvent *,      oldchain,       A0),
    AROS_UFHA(struct IIHData *,         iihdata,        A1)
)
{
    struct InputEvent	*ie;
    struct Screen	* screen;
    struct Gadget *gadget = iihdata->ActiveGadget;
    struct IntuitionBase *IntuitionBase = iihdata->IntuitionBase;
    ULONG  lock;
    char *ptr = NULL;
    WORD win_mousex, win_mousey;
    struct GadgetInfo *gi = &iihdata->GadgetInfo;
    BOOL reuse_event = FALSE;
    struct Window *w;
    
    D(bug("Inside intuition inputhandler, active window=%p\n", IntuitionBase->ActiveWindow));

    /* First handle IntuiMessages which were replied back to the IntuiReplyPort
       by the apps */
       
    HandleIntuiReplyPort(iihdata, IntuitionBase);
    
    /* Now handle the input events */
    
    for (ie = oldchain; ie; ie = ((reuse_event) ? ie : ie->ie_NextEvent))
    {
    
	struct Window *old_w;
	BOOL swallow_event = FALSE;
	BOOL new_active_window = FALSE;
    
    	D(bug("iih: Handling event of class %d, code %d\n", ie->ie_Class, ie->ie_Code));
	reuse_event = FALSE;
	ptr = NULL;

	iihdata->ActQualifier = ie->ie_Qualifier;
	
        /* Use event to find the active window */
             
        lock = LockIBase(0UL);
       
    	old_w = IntuitionBase->ActiveWindow;
	if (ie->ie_Class == IECLASS_RAWMOUSE && ie->ie_Code == SELECTDOWN)
	{
	    w = intui_FindActiveWindow(ie, &swallow_event, IntuitionBase);
	}
	else
	{
	    w = old_w;
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
		
 	UnlockIBase(lock);
		
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

		    gadget->Flags &= ~GFLG_SELECTED;
		    RefreshGList(gadget, gi->gi_Window, NULL, 1);
		    break;
		    
	        case GTYP_PROPGADGET:
		    /* That a prop gadget is active here can only happen
		       if user used LMB to activate gadget and LAMIGA + LALT
		       to activate other window, or viceversa */

		    HandlePropSelectUp(gadget, gi->gi_Window, NULL, IntuitionBase);
		    if (gadget->Activation & GACT_RELVERIFY)
		    {
			fire_intuimessage(gi->gi_Window,
			    		  IDCMP_GADGETUP,
					  0,
					  gadget,
					  IntuitionBase);
		    }
		   
		}
		
		gadget->Activation &= ~GACT_ACTIVEGADGET;
		iihdata->ActiveGadget = NULL;
		gadget = NULL;
		
	    } /* if (gadget) */

	    if(old_w)
	    {
		if (old_w->IDCMPFlags & IDCMP_INACTIVEWINDOW)
		{
		    fire_intuimessage(old_w,
				      IDCMP_INACTIVEWINDOW,
				      0,
				      old_w,
				      IntuitionBase);
		}
	    }

	    /* int_activatewindow works if w = NULL */
	    int_activatewindow(w, IntuitionBase);

	    if (w)
	    {
		if (w->IDCMPFlags & IDCMP_ACTIVEWINDOW)
		{
		    fire_intuimessage(w,
				      IDCMP_ACTIVEWINDOW,
				      0,
				      w,
				      IntuitionBase);
		}
	    }
	    
	    
	} /* if (new_active_window) */
		
	if (swallow_event)
	    continue;

        /* If there is no active window, nothing to do */
        if (w == NULL)
	    continue;
	         
	win_mousex = ie->ie_X - w->LeftEdge;
	win_mousey = ie->ie_Y - w->TopEdge;
 
	if (w->IDCMPFlags & IDCMP_DELTAMOVE)
	{
	  win_mousex -= w->MouseX;
	  win_mousey -= w->MouseY;
	}
		
 	/* 
	**  IntuiMessages get the mouse coordinates relative to
	**  the upper left corner of the window no matter if
	**  window is GZZ or not
	*/
			    
	screen = w->WScreen;
	
	switch (ie->ie_Class)
	{
	    
	case IECLASS_REFRESHWINDOW:
	    fire_intuimessage(w, IDCMP_REFRESHWINDOW, 0, w, IntuitionBase);

	    RefreshGadgets (w->FirstGadget, w, NULL);
	    
	    break; /* case IECLASS_REFRESHWINDOW */

	case IECLASS_SIZEWINDOW:
	    fire_intuimessage(w, IDCMP_NEWSIZE, 0, w, IntuitionBase);

	    /* Change width of dragbar gadget */


	    /* Send GM_LAYOUT to all GA_RelSpecial BOOPSI gadgets */
	    DoGMLayout(w->FirstGadget, w, NULL, -1, FALSE, IntuitionBase);
	    
	    break; /* case IECLASS_SIZEWINDOW */

	case IECLASS_RAWMOUSE:
	    switch (ie->ie_Code)
	    {
	    case SELECTDOWN: {
		BOOL new_gadget = FALSE;

		if (!gadget)
		{
  		    gadget = FindGadget (w, ie->ie_X, ie->ie_Y, gi, IntuitionBase);
		    if (gadget)
		    {
		        /* Whenever the active gadget changes the gi must be updated
			   because it is cached in iidata->GadgetInfo!!!! Don't
			   forget to do this if somewhere else the active
			   gadget is changed, for example in ActivateGadget!!! */
			   
		        PrepareGadgetInfo(gi, w);
		    	SetGadgetInfoGadget(gi, gadget);
			
			new_gadget = TRUE;
		    }
		}

		if (gadget)

		{
		    if ((gadget->Activation & GACT_IMMEDIATE) &&
			(w->IDCMPFlags & IDCMP_GADGETDOWN))
		    {
			fire_intuimessage(w,
					  IDCMP_GADGETDOWN,
					  0,
					  gadget,
					  IntuitionBase);

		    }

		    switch (gadget->GadgetType & GTYP_GTYPEMASK)
		    {
		    case GTYP_BOOLGADGET:
			if (gadget->Activation & GACT_TOGGLESELECT)
			    gadget->Flags ^= GFLG_SELECTED;
			else
			    gadget->Flags |= GFLG_SELECTED;
			    

			RefreshGList (gadget, w, NULL, 1);

			break;

		    case GTYP_PROPGADGET:
			GetGadgetDomain(gadget, w, NULL, &gi->gi_Domain);

			HandlePropSelectDown(gadget,
					     w,
					     NULL,
					     win_mousex - gi->gi_Domain.Left - GetGadgetLeft(gadget, w, NULL),
					     win_mousey - gi->gi_Domain.Top  - GetGadgetTop(gadget, w, NULL),
					     IntuitionBase);


			break;

		    case GTYP_STRGADGET:
			/* If the click was inside the active strgad,
			** then let it update cursor pos,
			** else deactivate stringadget and reuse event.
			*/

			if (InsideGadget(w, gadget, ie->ie_X, ie->ie_Y))
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

		    case GTYP_CUSTOMGADGET: {
			struct gpInput gpi;
			IPTR retval;
			ULONG termination;
			
			if (new_gadget)
			{			   
			    gpi.MethodID = GM_GOACTIVE;				
			}
			else
			{
			    gpi.MethodID = GM_HANDLEINPUT;
			}
			
			gpi.gpi_GInfo	= gi;
			gpi.gpi_IEvent	= ie;
			gpi.gpi_Termination = &termination;
			gpi.gpi_Mouse.X = win_mousex - gi->gi_Domain.Left - GetGadgetLeft(gadget, w, NULL);
			gpi.gpi_Mouse.Y = win_mousey - gi->gi_Domain.Top  - GetGadgetTop(gadget, w, NULL);
			gpi.gpi_TabletData	= NULL;


			retval = Locked_DoMethodA ((Object *)gadget, (Msg)&gpi, IntuitionBase);

			gadget = HandleCustomGadgetRetVal(retval, gi, gadget, &termination, 
							  &reuse_event, IntuitionBase);
			    
			break; }


		    } /* switch (GadgetType) */

		} /* if (a gadget is active) */
		else
		{
		    fire_intuimessage(w,
		    		      IDCMP_MOUSEBUTTONS,
				      SELECTDOWN,
				      w,
				      IntuitionBase);
		}
		
		}break; /* case SELECTDOWN */

	    case SELECTUP:
		if (gadget)
		{
		    int inside = InsideGadget(w,gadget, ie->ie_X, ie->ie_Y);
		    int selected = (gadget->Flags & GFLG_SELECTED) != 0;


		    switch (gadget->GadgetType & GTYP_GTYPEMASK)
		    {
		    case GTYP_BOOLGADGET:
			if (!(gadget->Activation & GACT_TOGGLESELECT) )
			    gadget->Flags &= ~GFLG_SELECTED;

			if (selected)
			    RefreshGList (gadget, w, NULL, 1);

			if (inside && (gadget->Activation & GACT_RELVERIFY))
			{
			    fire_intuimessage(w,
			    		      IDCMP_GADGETUP,
					      0,
					      gadget,
					      IntuitionBase);
			} else {
			    /* RKRM say so */
			    fire_intuimessage(w,
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
			    fire_intuimessage(w,
			    		      IDCMP_GADGETUP,
					      0,
					      gadget,
					      IntuitionBase);
			}
			

			gadget = NULL;
			break;

		    /* Intuition string gadgets don't care about SELECTUP */

		    case GTYP_CUSTOMGADGET: {
			struct gpInput gpi;
			IPTR retval;
			ULONG termination;

			gpi.MethodID	= GM_HANDLEINPUT;
			gpi.gpi_GInfo	= gi;
			gpi.gpi_IEvent	= ie;
			gpi.gpi_Termination = &termination;
			gpi.gpi_Mouse.X = win_mousex - gi->gi_Domain.Left - GetGadgetLeft(gadget, w, NULL);
			gpi.gpi_Mouse.Y = win_mousey - gi->gi_Domain.Top  - GetGadgetTop(gadget, w, NULL);
			gpi.gpi_TabletData	= NULL;

			retval = Locked_DoMethodA ((Object *)gadget, (Msg)&gpi, IntuitionBase);

			gadget = HandleCustomGadgetRetVal(retval, gi, gadget, &termination,
							  &reuse_event, IntuitionBase);

			break; }

		    } /* switch GadgetType */

		} /* if (a gadget is currently active) */
		else
		{
		    fire_intuimessage(w,
		    		      IDCMP_MOUSEBUTTONS,
				      SELECTUP,
				      w,
				      IntuitionBase);
		}

		break; /* case SELECTUP */

	    case MENUDOWN:
	    case MENUUP:
	    case MIDDLEDOWN:
	    case MIDDLEUP:
		if (gadget)
		{
		    if ( (gadget->GadgetType & GTYP_GTYPEMASK) ==  GTYP_CUSTOMGADGET)
		    {

			struct gpInput gpi;
			IPTR retval;
			ULONG termination;
			
			gpi.MethodID	    = GM_HANDLEINPUT;
			gpi.gpi_GInfo	    = gi;
			gpi.gpi_IEvent	    = ie;
			gpi.gpi_Termination = &termination;
			gpi.gpi_Mouse.X     = win_mousex - gi->gi_Domain.Left - GetGadgetLeft(gadget, w, NULL);
			gpi.gpi_Mouse.Y     = win_mousey - gi->gi_Domain.Top  - GetGadgetTop(gadget, w, NULL);
			gpi.gpi_TabletData  = NULL;

			retval = Locked_DoMethodA((Object *)gadget, (Msg)&gpi, IntuitionBase);

			gadget = HandleCustomGadgetRetVal(retval, gi, gadget, &termination, 
							  &reuse_event, IntuitionBase);

		    } /* if (active gadget is a BOOPSI gad) */

		} /* if (there is an active gadget) */
		else
		{
		    fire_intuimessage(w,
		    		      IDCMP_MOUSEBUTTONS,
				      ie->ie_Code,
				      w,
				      IntuitionBase);
		}

		break; /* case MENUDOWN */

	    case IECODE_NOBUTTON: { /* MOUSEMOVE */

		iihdata->LastMouseX = ie->ie_X;
		iihdata->LastMouseY = ie->ie_Y;

		notify_mousemove_screensandwindows(ie->ie_X, 
		                                   ie->ie_Y, 
		                                   IntuitionBase);

		if (gadget)
		{
		    int inside = InsideGadget(w,gadget,ie->ie_X,ie->ie_Y);
		    int selected = (gadget->Flags & GFLG_SELECTED) != 0;

		    switch (gadget->GadgetType & GTYP_GTYPEMASK)
		    {
		    case GTYP_BOOLGADGET:
			if  (inside != selected)
			{
			    gadget->Flags ^= GFLG_SELECTED;
			    RefreshGList (gadget, w, NULL, 1);
			}
			break;

		    case GTYP_PROPGADGET:
		        GetGadgetDomain(gadget, w, NULL, &gi->gi_Domain);
			
			HandlePropMouseMove(gadget
				,w
				,NULL
				,win_mousex - gi->gi_Domain.Left - GetGadgetLeft(gadget, w, NULL)
				,win_mousey - gi->gi_Domain.Top  - GetGadgetTop(gadget, w, NULL)
				,IntuitionBase);

			break;

		    case GTYP_CUSTOMGADGET: {
			struct gpInput gpi;
			IPTR retval;
			ULONG termination;
			
			gpi.MethodID	= GM_HANDLEINPUT;
			gpi.gpi_GInfo	= gi;
			gpi.gpi_IEvent	= ie;
			gpi.gpi_Termination = &termination;
			gpi.gpi_Mouse.X     = win_mousex - gi->gi_Domain.Left - GetGadgetLeft(gadget, w, NULL);
			gpi.gpi_Mouse.Y     = win_mousey - gi->gi_Domain.Top  - GetGadgetTop(gadget, w, NULL);
			gpi.gpi_TabletData  = NULL;

			retval = Locked_DoMethodA ((Object *)gadget, (Msg)&gpi, IntuitionBase);

			gadget = HandleCustomGadgetRetVal(retval, gi, gadget, &termination, 
							  &reuse_event, IntuitionBase);
			
			break; }

		    } /* switch GadgetType */
	    	
		} /* if (a gadget is currently active) */

		if (!(w->IDCMPFlags & IDCMP_MOUSEMOVE)) continue;
		
		/* Send IDCMP_MOUSEMOVE if WFLG_REPORTMOUSE is set
		   and/or active gadget has GACT_FOLLOWMOUSE set */
		   
		if (!(w->Flags & WFLG_REPORTMOUSE))
		{
		    if (!gadget) continue;
		    if (!(gadget->Activation & GACT_FOLLOWMOUSE)) continue;
		}
		
		/* Limit the number of IDCMP_MOUSEMOVE messages sent to intuition.
		   note that this comes after handling gadgets, because gadgets should get all events.
		*/

		if (IW(w)->num_mouseevents >= IW(w)->mousequeue)
		{
		    /* Mouse Queue is full, so don't send any more IDCMP_MOUSEMOVE
		       messages */
		    continue;
		}
		
		/* MouseQueue is not full, so we can send a message. We increase
		   IntWindow->num_mouseevents which will later be decreased after
		   the Intuition InputHandler gets the ReplyMessage from the app
		   and handles it in HandleIntuiReplyPort() */
		
		IW(w)->num_mouseevents++;
		
		fire_intuimessage(w,
				  IDCMP_MOUSEMOVE,
				  IECODE_NOBUTTON,
				  w,
				  IntuitionBase);
				  
	        break; } /* case IECODE_NOBUTTON */

	    } /* switch (im->im_Code)  (what button was pressed ?) */
	    break;


	case IECLASS_RAWKEY:
	    /* send release events only to windows who
	       have not set IDCMP_VANILLAKEY and no
	       active gadget */
	       
	    if ( (!(ie->ie_Code & IECODE_UP_PREFIX)) ||
	         (!gadget && ((w->IDCMPFlags & IDCMP_VANILLAKEY) == 0)) )
	    {
		ptr = "RAWKEY PRESSED";

		if (gadget)
		{
		    switch (gadget->GadgetType & GTYP_GTYPEMASK)
		    {
		    case GTYP_STRGADGET: {
			UWORD imsgcode;
			ULONG ret = HandleStrInput(gadget, gi, ie, &imsgcode,
						   IntuitionBase);

			if (ret & SGA_END)
			{
			    if (gadget->Activation & GACT_RELVERIFY)
			    {
			        fire_intuimessage(w,
						  IDCMP_GADGETUP,
						  imsgcode,
						  gadget,
						  IntuitionBase);
				gadget = NULL;

			    }
			}

			break; }

		    case GTYP_CUSTOMGADGET: {
			struct gpInput gpi;
			IPTR retval;
			ULONG termination;
			
			gpi.MethodID	    = GM_HANDLEINPUT;
			gpi.gpi_GInfo	    = gi;
			gpi.gpi_IEvent	    = ie;
			gpi.gpi_Termination = &termination;
			gpi.gpi_Mouse.X     = win_mousex - gi->gi_Domain.Left - GetGadgetLeft(gadget, w, NULL);
			gpi.gpi_Mouse.Y     = win_mousey - gi->gi_Domain.Top  - GetGadgetTop(gadget, w, NULL);
			gpi.gpi_TabletData  = NULL;

			retval = Locked_DoMethodA((Object *)gadget, (Msg)&gpi, IntuitionBase);

			gadget = HandleCustomGadgetRetVal(retval, gi, gadget, &termination,
							  &reuse_event, IntuitionBase);


			break;}  /* case BOOPSI custom gadget type */

		    } /* switch (gadget type) */

		} /* if (a gadget is currently active) */
		else
		{
		    /* This is a regular RAWKEY event (no gadget taking care
		       of it...). */

		    if(w->IDCMPFlags & IDCMP_VANILLAKEY)
		    {
			UBYTE keyBuffer;

			if(MapRawKey(ie, &keyBuffer, 1, NULL) == 1)
			{
			    fire_intuimessage(w,
			    		      IDCMP_VANILLAKEY,
					      keyBuffer,
					      0,
					      IntuitionBase);
			    break;
			} 

			/* If the event mapped to more than one byte, it is not
			   a legal VANILLAKEY, so we send it as the original
			   RAWKEY event. */
			       			    
		    }
		    
		    fire_intuimessage(w,
		    		      IDCMP_RAWKEY,
				      ie->ie_Code,
				      0, /* TODO: Should be prevdownqode/qual */
				      IntuitionBase);
				      
		} /* regular RAWKEY */
		
	    }
	    break; /* case IECLASS_RAWKEY */
	    
	case IECLASS_TIMER:	    
	    if (gadget)
	    {
	        if ((gadget->GadgetType & GTYP_GTYPEMASK) == GTYP_CUSTOMGADGET)
		{
		    /* Pass the event to the active gadget */
		    struct gpInput gpi;
		    IPTR retval;
		    ULONG termination;
		    
		    gpi.MethodID	= GM_HANDLEINPUT;
		    gpi.gpi_GInfo	= gi;
		    gpi.gpi_IEvent	= ie;
		    gpi.gpi_Termination = &termination;
		    gpi.gpi_Mouse.X     = win_mousex - gi->gi_Domain.Left - GetGadgetLeft(gadget, w, NULL);
		    gpi.gpi_Mouse.Y     = win_mousey - gi->gi_Domain.Top  - GetGadgetTop(gadget, w, NULL);
		    gpi.gpi_TabletData  = NULL;

		    retval = Locked_DoMethodA((Object *)gadget, (Msg)&gpi, IntuitionBase);

		    gadget = HandleCustomGadgetRetVal(retval, gi, gadget, &termination,
						      &reuse_event, IntuitionBase);

		} /* if ((gadget->GadgetType & GTYP_GTYPEMASK) == GTYP_CUSTOMGADGET) */
		
	    } /* if (gadget) */
	    
	    /* Send INTUITICK msg only if app already replied the last INTUITICK msg */
	    if (w->Flags & WFLG_WINDOWTICKED) continue;
	    
	    /* Set the WINDOWTICKED flag, it will be cleared again when the app
	       replies back the msg and the InputHandler handles the replymsg
	       in HandleIntuiReplyPort() */
	       
	    fire_intuimessage(w,
	    		      IDCMP_INTUITICKS,
			      0,
			      w,
			      IntuitionBase);
			      
	    Forbid();
	    w->Flags |= WFLG_WINDOWTICKED;
	    Permit();
	    
	    break; /* case IECLASS_TIMER */

	case IECLASS_ACTIVEWINDOW:
	    fire_intuimessage(w, IDCMP_ACTIVEWINDOW, 0, w, IntuitionBase);
	    break;

	case IECLASS_INACTIVEWINDOW:
	    fire_intuimessage(w, IDCMP_INACTIVEWINDOW, 0, w, IntuitionBase);
	    break;

	default:
	    ptr = NULL;

            kprintf("Unknown IEClass!\n");
	    break;
	} /* switch (im->Class) */


    } /* for (each event in the chain) */

    iihdata->ActiveGadget = gadget;


    HandleDeferedActions(iihdata, IntuitionBase);
    
    D(bug("Outside pollingloop\n"));
    return (oldchain);
}




