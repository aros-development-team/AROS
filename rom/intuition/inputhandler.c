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

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

#define IW(x) ((struct IntWindow *)(x))

inline VOID send_intuimessage(struct IntuiMessage *imsg, struct Window *w,
			      struct IntuitionBase *IntuitionBase);

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


static BOOL InsideGadget(struct Window *win, struct Gadget *gad,
			 WORD x, WORD y)
{
    struct IBox box;
    BOOL rc = FALSE;
    
    GetScrGadgetIBox(gad, win, NULL, &box);
       
    if ((x >= box.Left) &&
    	(y >= box.Top)  &&
	(x < box.Left + box.Width) &&
	(y < box.Top + box.Height))
    {
    	rc = TRUE;
    }
    
    return rc; 
}


/******************************
 **  NotifyDepthArrangement  **
 *****************************/

static void NotifyDepthArrangement(struct Window *w,
				   struct IntuitionBase *IntuitionBase)
{
    if(w->MoreFlags & WMFLG_NOTIFYDEPTH)
    {
	struct IntuiMessage *im = alloc_intuimessage(IntuitionBase);
	
	if(!im)
	    return;
	
	im->Code = CWCODE_DEPTH;
	im->Class = IDCMP_CHANGEWINDOW;
	
	send_intuimessage(im, w, IntuitionBase);
    }    
}


/*************************
**  Locked_DoMethodA	**
*************************/
static IPTR Locked_DoMethodA (Object * obj, Msg message, struct IntuitionBase *IntuitionBase)
{
    IPTR rc;
    
    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock);
    rc = DoMethodA(obj, message);
    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock);
    
    return rc;
}

/*****************
**  FindGadget	**
*****************/
static struct Gadget * FindGadget (struct Window * window, int x, int y,
			           struct GadgetInfo * gi, struct IntuitionBase *IntuitionBase)
{
    struct Gadget * gadget;
    struct gpHitTest gpht;
    struct IBox ibox;
    WORD xrel, yrel;

    gpht.MethodID     = GM_HITTEST;
    gpht.gpht_GInfo   = gi;

    for (gadget=window->FirstGadget; gadget; gadget=gadget->NextGadget)
    {
    	/* stegerg: domain depends on gadgettype and windowflags! */
        GetGadgetDomain(gadget, window, NULL, &gi->gi_Domain);
	
	/* Get coords relative to window */

    	GetGadgetIBox((Object *)gadget, gi, &ibox);

	xrel = x - gi->gi_Domain.Left - window->LeftEdge;
	yrel = y - gi->gi_Domain.Top  - window->TopEdge;

	if (   xrel >= ibox.Left
	    && yrel >= ibox.Top
	    && xrel < ibox.Left + ibox.Width 
	    && yrel < ibox.Top  + ibox.Height )
	{
	    if ((gadget->GadgetType & GTYP_GTYPEMASK) != GTYP_CUSTOMGADGET) break;
	     
	    gpht.gpht_Mouse.X = xrel - ibox.Left;
	    gpht.gpht_Mouse.Y = yrel - ibox.Top;

	    if (Locked_DoMethodA ((Object *)gadget, (Msg)&gpht, IntuitionBase) == GMR_GADGETHIT)
		break;
	}

    }

    return (gadget);
} /* FindGadget */


/************************
**  PrepareGadgetInfo  **
************************/
static void PrepareGadgetInfo(struct GadgetInfo *gi, struct Window *win)
{
    gi->gi_Screen	  = win->WScreen;
    gi->gi_Window	  = win;
    gi->gi_Domain	  = *((struct IBox *)&win->LeftEdge); /* depends on gadget: will be overwritten */
    gi->gi_RastPort   	  = win->RPort;			      /* "       "  "     : "    "  "           */
    gi->gi_Pens.DetailPen = gi->gi_Screen->DetailPen;
    gi->gi_Pens.BlockPen  = gi->gi_Screen->BlockPen;
    gi->gi_DrInfo	  = &(((struct IntScreen *)gi->gi_Screen)->DInfo);
}


/**************************
**  SetGadgetInfoGadget  **
**************************/
static void SetGadgetInfoGadget(struct GadgetInfo *gi, struct Gadget *gad)
{
    SET_GI_RPORT(gi, gi->gi_Window, gad);
    GetGadgetDomain(gad, gi->gi_Window, NULL, &gi->gi_Domain);
}


/*******************************
**  HandleCustomGadgetRetVal  **
*******************************/
static struct Gadget *HandleCustomGadgetRetVal(IPTR retval, struct GadgetInfo *gi, struct Gadget *gadget,
					       struct IntuiMessage *im, ULONG *termination, char **ptr,
					       BOOL *reuse_event,struct IntuitionBase *IntuitionBase)
{					       
    if (retval != GMR_MEACTIVE)
    {
	struct gpGoInactive gpgi;

	if (retval & GMR_REUSE)
	    *reuse_event = TRUE;

	if (    (retval & GMR_VERIFY)
	     && (gadget->Activation & GACT_RELVERIFY))
	{
	    im->Class 	 = IDCMP_GADGETUP;
	    im->IAddress = gadget;
	    *ptr	 = "GADGETUP";
	    im->Code 	 = (*termination) & 0x0000FFFF;
	}
	else
	{
	    im->Class = 0; /* Swallow event */
	}

	gpgi.MethodID = GM_GOINACTIVE;
	gpgi.gpgi_GInfo = gi;
	gpgi.gpgi_Abort = 0;

	Locked_DoMethodA((Object *)gadget, (Msg)&gpgi, IntuitionBase);

	gadget->Activation &= ~GACT_ACTIVEGADGET;
	gadget = NULL;
    } /* if (retval != GMR_MEACTIVE) */
    else
    {
	gadget->Activation |= GACT_ACTIVEGADGET;
    }
    
    return gadget;
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
    struct IntuiMessage *im = NULL;
    struct DeferedActionMessage *am;
    struct Screen	* screen;
    struct Gadget *gadget = iihdata->ActiveGadget;
    struct IntuitionBase *IntuitionBase = iihdata->IntuitionBase;
    ULONG  lock;
    char *ptr = NULL;
    WORD win_mousex, win_mousey;
    struct GadgetInfo stackgi, *gi = &stackgi;
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
	    /* int_activatewindow works if w = NULL */
	    int_activatewindow(w, IntuitionBase);
	    
	    /* If there was an active gadget in the old window
	       we must make it inactive */
	    
	    if (gadget)
	    {
	    	switch (gadget->GadgetType & GTYP_GTYPEMASK)
		{
		
		case GTYP_CUSTOMGADGET:
		    {
		    	struct gpGoInactive gpgi;
			
		    	PrepareGadgetInfo(gi, old_w);
		    	SetGadgetInfoGadget(gi, gadget);
			
			gpgi.MethodID = GM_GOINACTIVE;
			gpgi.gpgi_GInfo = gi;
			gpgi.gpgi_Abort = 1; 
			
			Locked_DoMethodA((Object *)gadget, (Msg)&gpgi, IntuitionBase);
		    }
		    break;
		}
		
		gadget->Activation &= ~GACT_ACTIVEGADGET;
		iihdata->ActiveGadget = NULL;
		gadget = NULL;
		
	    } /* if (gadget) */
	    
	} /* if (new_active_window) */
		
	if (swallow_event)
	    continue;

        /* If there is no active window, nothing to do */
        if (w == NULL)
	    continue;
	         
	/* mouse position relative to upper left window edge,
	   only valid for certain IECLASSes!! */
	   
	win_mousex = ie->ie_X - w->LeftEdge;
	win_mousey = ie->ie_Y - w->TopEdge;
		
	/*
	** If the last InputEvent was swallowed, we can reuse the IntuiMessage.
	** If it was sent to an app, then we have to get a new IntuiMessage
	*/
	
	if (!im)
	{
	    im = alloc_intuimessage(IntuitionBase);
	}
	    
	if (!im)
	{
	    /* Ouch, we're in BIG trouble */
	    Alert(AT_DeadEnd|AN_Intuition|AG_NoMemory);
	}

 	/* 
	**  IntuiMessages get the mouse coordinates relative to
	**  the upper left corner of the window no matter if
	**  window is GZZ or not
	*/
		
	im->Class	= 0L;
	im->IAddress	= NULL;
	im->MouseX	= win_mousex;
	im->MouseY	= win_mousey;
	im->IDCMPWindow = w;
	    
	    
	screen = w->WScreen;

        /* setup GadgetInfo */
	
	PrepareGadgetInfo(gi, w);
	if (gadget) SetGadgetInfoGadget(gi, gadget);
	
	switch (ie->ie_Class)
	{
	    
	case IECLASS_REFRESHWINDOW:
	    ptr       = "REFRESHWINDOW";
	    im->Class = IDCMP_REFRESHWINDOW;

	    RefreshGadgets (w->FirstGadget, w, NULL);
	    
	    break; /* case IECLASS_REFRESHWINDOW */

	case IECLASS_SIZEWINDOW:
	    ptr       = "NEWSIZE";
	    im->Class = IDCMP_NEWSIZE;

	    /* Change width of dragbar gadget */


	    /* Send GM_LAYOUT to all GA_RelSpecial BOOPSI gadgets */
	    DoGMLayout(w->FirstGadget, w, NULL, -1, FALSE, IntuitionBase);
	    
	    break; /* case IECLASS_SIZEWINDOW */

	case IECLASS_RAWMOUSE:
	    /* IECLASS_RAWMOUSE events are let through even when there
	       is no active window */
	    if (im)
	    {
		im->Code	= ie->ie_Code;
	    }
	    

	    ptr = "RAWMOUSE";

	    switch (ie->ie_Code)
	    {
	    case SELECTDOWN: {
		BOOL new_gadget = FALSE;

		im->Class = IDCMP_MOUSEBUTTONS;
		ptr = "MOUSEBUTTONS";

		if (!gadget)
		{
  		    gadget = FindGadget (w, ie->ie_X, ie->ie_Y, gi, IntuitionBase);
		    if (gadget)
		    {
		    	SetGadgetInfoGadget(gi, gadget);
			new_gadget = TRUE;
		    }
		}

		if (gadget)
		{
		    if (gadget->Activation & GACT_IMMEDIATE)
		    {
			im->Class	= IDCMP_GADGETDOWN;
			im->IAddress	= gadget;
			ptr		= "GADGETDOWN";
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
			
			/* Georg Steger: two idcmp_gadgetdowns were sent,
			   unless goactive returns something != GMR_MEACTIVE */			
			im->Class = 0L;
			
			/* We should send IDCMP_GADGETDOWN for GACT_IMMEDIATE gadgets */
			if (new_gadget)
			{
			    
			    if ((gadget->Activation & GACT_IMMEDIATE) &&
			        (w->IDCMPFlags & IDCMP_GADGETDOWN))
			    {
			    	struct IntuiMessage *imsg;
				
				imsg = alloc_intuimessage(IntuitionBase);
				if (!imsg)
				{
				
				     /* If we can't send IDCMPGADGETDOWN we do not activate this gadget */
				     gadget = NULL;
				     break;
				}
				     
				imsg->Class = IDCMP_GADGETDOWN;
				imsg->IAddress = gadget;
				
				send_intuimessage(imsg, w, IntuitionBase);
			    }


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

			gadget = HandleCustomGadgetRetVal(retval, gi, gadget, im, &termination, &ptr,
							  &reuse_event, IntuitionBase);
			    
			break; }


		    } /* switch (GadgetType) */

		} /* if (a gadget is active) */

		if (im->Class == IDCMP_MOUSEBUTTONS)
		    ptr = "MOUSEBUTTONS";

		}break; /* case SELECTDOWN */

	    case SELECTUP:
		im->Class = IDCMP_MOUSEBUTTONS;
		ptr = "MOUSEBUTTONS";

		D(bug("SELECTUP\n"));
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
			    im->Class	 = IDCMP_GADGETUP;
			    im->IAddress = gadget;
			    ptr = "GADGETUP";
			}

			gadget = NULL;
			break;

		    case GTYP_PROPGADGET:
			HandlePropSelectUp(gadget, w, NULL, IntuitionBase);
			if (inside && (gadget->Activation & GACT_RELVERIFY))
			{
			    im->Class	 = IDCMP_GADGETUP;
			    im->IAddress = gadget;
			    ptr = "GADGETUP";
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

			gadget = HandleCustomGadgetRetVal(retval, gi, gadget, im, &termination, &ptr,
							  &reuse_event, IntuitionBase);

			break; }

		    } /* switch GadgetType */

		} /* if (a gadget is currently active) */

		break; /* case SELECTUP */

	    case MENUDOWN:
		im->Class = IDCMP_MOUSEBUTTONS;
		ptr = "MOUSEBUTTONS";

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

			gadget = HandleCustomGadgetRetVal(retval, gi, gadget, im, &termination, &ptr,
							  &reuse_event, IntuitionBase);

		    } /* if (active gadget is a BOOPSI gad) */

		} /* if (there is an active gadget) */
		break; /* case MENUDOWN */

	    case MENUUP:
		im->Class = IDCMP_MOUSEBUTTONS;
		ptr = "MOUSEBUTTONS";

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

			gadget = HandleCustomGadgetRetVal(retval, gi, gadget, im, &termination, &ptr,
							  &reuse_event, IntuitionBase);

		    } /* if (active gadget is a BOOPSI gad) */

		} /* if (there is an active gadget) */

		break; /* case MENUUP */


	    case IECODE_NOBUTTON: { /* MOUSEMOVE */
		struct IntuiMessage *msg, *succ;

		im->Class = IDCMP_MOUSEMOVE;
	        
		ptr = "MOUSEMOVE";
		iihdata->LastMouseX = ie->ie_X;
		iihdata->LastMouseY = ie->ie_Y;
		
		/* Set the screens mouse coords.
		   This won't work if no window is active */
		w->WScreen->MouseX = ie->ie_X;
		w->WScreen->MouseY = ie->ie_Y;

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

			gadget = HandleCustomGadgetRetVal(retval, gi, gadget, im, &termination, &ptr,
							  &reuse_event, IntuitionBase);
			
			break; }

		    } /* switch GadgetType */
	    	
		} /* if (a gadget is currently active) */

		if (!(w->Flags & WFLG_REPORTMOUSE)) continue;
		
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
		
	        break; } /* case IECODE_NOBUTTON */

	    } /* switch (im->im_Code)  (what button was pressed ?) */
	    break;


	case IECLASS_RAWKEY:
	    im->Class	    = IDCMP_RAWKEY;
	    im->Code	    = ie->ie_Code;
	    im->Qualifier   = ie->ie_Qualifier;

	    if (!(ie->ie_Code & IECODE_UP_PREFIX))
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

			im->Class = 0;      /* Already used for strgadget. */

			if (ret & SGA_END)
			{
			    if (gadget->Activation & GACT_RELVERIFY)
			    {
				im->Class = IDCMP_GADGETUP;
				im->Code  = imsgcode;
				im->IAddress = gadget;
				gadget = NULL;

				ptr = "GADGETUP";
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
			gpi.gpi_Mouse.X     = im->MouseX - gi->gi_Domain.Left - GetGadgetLeft(gadget, w, NULL);
			gpi.gpi_Mouse.Y     = im->MouseY - gi->gi_Domain.Top  - GetGadgetTop(gadget, w, NULL);
			gpi.gpi_TabletData  = NULL;

			retval = Locked_DoMethodA((Object *)gadget, (Msg)&gpi, IntuitionBase);

			gadget = HandleCustomGadgetRetVal(retval, gi, gadget, im, &termination, &ptr,
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

			if(MapRawKey(ie, &keyBuffer, 1, NULL) != -1)
			{
			    im->Class = IDCMP_VANILLAKEY;
			    im->Code  = keyBuffer;
			}

			/* If the event mapped to more than one byte, it is not
			   a legal VANILLAKEY, so we send it as the original
			   RAWKEY event. */
		    }
		    
		} /* regular RAWKEZ */
		
	    }
	    else /* key released */
	    {
		ptr = "RAWKEY RELEASED";
	    }
	    break; /* case IECLASS_RAWKEY */
	    
	case IECLASS_TIMER:
	    im->Class = IDCMP_INTUITICKS;
	    ptr = "INTUITICK";
	    
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
		    gpi.gpi_Mouse.X     = im->MouseX - gi->gi_Domain.Left - GetGadgetLeft(gadget, w, NULL);
		    gpi.gpi_Mouse.Y     = im->MouseY - gi->gi_Domain.Top  - GetGadgetTop(gadget, w, NULL);
		    gpi.gpi_TabletData  = NULL;

		    retval = Locked_DoMethodA((Object *)gadget, (Msg)&gpi, IntuitionBase);

		    gadget = HandleCustomGadgetRetVal(retval, gi, gadget, im, &termination, &ptr,
						      &reuse_event, IntuitionBase);

		} /* if ((gadget->GadgetType & GTYP_GTYPEMASK) == GTYP_CUSTOMGADGET) */
		
	    } /* if (gadget) */
	    
	    /* Send INTUITICK msg only if app already replied the last INTUITICK msg */
	    if (w->Flags & WFLG_WINDOWTICKED) continue;
	    
	    /* Set the WINDOWTICKED flag, it will be cleared again when the app
	       replies back the msg and the InputHandler handles the replymsg
	       in HandleIntuiReplyPort() */
	       
	    Forbid();
	    w->Flags |= WFLG_WINDOWTICKED;
	    Permit();
	    
	    break; /* case IECLASS_TIMER */

	case IECLASS_ACTIVEWINDOW:
	    im->Class = IDCMP_ACTIVEWINDOW;
	    ptr = "ACTIVEWINDOW";
	    break;

	case IECLASS_INACTIVEWINDOW:
	    im->Class = IDCMP_INACTIVEWINDOW;
	    ptr = "INACTIVEWINDOW";
	    break;

	default:
	    ptr = NULL;

            kprintf("Unknown IEClass!\n");
	    break;
	} /* switch (im->Class) */


	if (ptr)
	     D(bug("Msg=%s\n", ptr));
	     
D(bug("Window: %p\n", w));

	 if (im->Class)
	 {
	    if ((im->Class & w->IDCMPFlags) && w->UserPort)
	    {
		im->ExecMessage.mn_ReplyPort = iihdata->IntuiReplyPort;

		send_intuimessage(im, w, IntuitionBase);
		    
		im = NULL;
		D(bug("Msg put\n"));

	    }
	    else
		im->Class = 0;
	}

    } /* for (each event in the chain) */

    iihdata->ActiveGadget = gadget;


    if (im)
    {
	free_intuimessage(im, IntuitionBase);
	im = NULL;
    }

    D(bug("Handle defered action messages\n"));
    
    /* Handle defered action messages */
    while ((am = (struct DeferedActionMessage *)GetMsg (iihdata->IntuiDeferedActionPort)))
    {
        BOOL CheckLayersBehind = FALSE;
        BOOL CheckLayersInFront = FALSE;
        struct Window * targetwindow = am->Window;
        struct Layer  * targetlayer  = targetwindow->WLayer;
        struct Layer * L;

	switch (am->Code)
	{
	    case AMCODE_CLOSEWINDOW: {
		struct closeMessage *cmsg = (struct closeMessage *)am;

		if (0 == (targetwindow->Flags & WFLG_GIMMEZEROZERO))
		{
		  /* not a GGZ window */
		  L = targetlayer->back;
		}
		else
		{
		  /* a GZZ window */
		  L = targetlayer->back->back;
		}

		if (NULL != L)
		  CheckLayersBehind = TRUE;
		int_closewindow(cmsg, IntuitionBase);
	    break; }

	    case AMCODE_WINDOWTOFRONT: {
		if (0 == (targetlayer->Flags & LAYERBACKDROP))
		{
		  /* GZZ or regular window? */
		  if (0 != (targetwindow->Flags & WFLG_GIMMEZEROZERO))
		  {
		    /* bring outer window to front first!! */

		    UpfrontLayer(NULL, targetwindow->BorderRPort->Layer);
		    if (targetwindow->BorderRPort->Layer->Flags & LAYERREFRESH)
		    {
			BeginUpdate(targetwindow->BorderRPort->Layer);
		        RefreshWindowFrame(targetwindow);
			EndUpdate(targetwindow->BorderRPort->Layer, TRUE);

			targetwindow->BorderRPort->Layer->Flags &= ~LAYERREFRESH;
		    }
		  }

		  UpfrontLayer(NULL, targetlayer);

		  /* only this layer (inner window) needs to be updated */
		  if (0 != (targetlayer->Flags & LAYERREFRESH))
		  {
		    struct IntuiMessage * IM;

		    /* stegerg */

		    BeginUpdate(targetlayer);
		    if (0 == (targetwindow->Flags & WFLG_GIMMEZEROZERO))
		    {
			RefreshWindowFrame(targetwindow);
		    }
		    RefreshGadgets(targetwindow->FirstGadget, w, NULL);
		    EndUpdate(targetlayer, FALSE);

		    if (targetwindow->IDCMPFlags & IDCMP_REFRESHWINDOW)
		    {
			IM = alloc_intuimessage(IntuitionBase);
			targetlayer->Flags &= ~LAYERREFRESH;
			if (NULL != IM)
			{
			  IM->Class = IDCMP_REFRESHWINDOW;
			  send_intuimessage(IM, targetwindow, IntuitionBase);
			}
		    }
		  }
		} 
		
		NotifyDepthArrangement(targetwindow, IntuitionBase);
		FreeMem(am, sizeof(struct DeferedActionMessage));
	    break; }

	    case AMCODE_WINDOWTOBACK: {
		/* I don't move backdrop layers! */
		if (0 == (targetlayer->Flags & LAYERBACKDROP))
		{

		  BehindLayer(0, targetlayer);


		  /* GZZ window or regular window? */
		  if (0 != (targetwindow->Flags & WFLG_GIMMEZEROZERO))
		  {
		      /* move outer window behind! */
		      /* attention: targetlayer->back would not be valid as
		                    targetlayer already moved!! 
		      */
		      BehindLayer(0, targetwindow->BorderRPort->Layer);

		      /* 
                       * stegerg: check all layers including inner gzz
		       *          layer because of this: inner layer
		       *          was moved back first, so it gets 
		       *          completely under the outer layer.
		       *          Maybe it would be better to move the
		       *          outer layer first and then move the
		       *          inner layer with MoveLayerinfrontof
		       *          (outerlayer) ????
		       * 
                       */

		      L = targetlayer; /* targetlayer->front */
		      CheckLayersInFront = TRUE;

		  } else {

                      /* 
                       * Check all layers in front of the layer.
                       */
		      L = targetlayer->front;
		      CheckLayersInFront = TRUE;
		  }
		}

		NotifyDepthArrangement(targetwindow, IntuitionBase);
		FreeMem(am, sizeof(struct DeferedActionMessage));
	    break; }

	    case AMCODE_ACTIVATEWINDOW: {
		int_activatewindow(targetwindow, IntuitionBase);

		FreeMem(am, sizeof (struct DeferedActionMessage));
	    break; }


            case AMCODE_MOVEWINDOW: { 

                 MoveLayer(0,
                           targetlayer,
                           am->dx,
                           am->dy);

                 /* in case of GZZ windows also move outer window */
                 if (0 != (targetwindow->Flags & WFLG_GIMMEZEROZERO))
                 {
                   MoveLayer(NULL,
                             targetwindow->BorderRPort->Layer,
                             am->dx,
                             am->dy);
                   RefreshWindowFrame(targetwindow);
                 }


                 ((struct IntWindow *)targetwindow)->ZipLeftEdge = targetwindow->LeftEdge;
                 ((struct IntWindow *)targetwindow)->ZipTopEdge  = targetwindow->TopEdge;

                 targetwindow->LeftEdge += am->dx;
                 targetwindow->TopEdge  += am->dy;

                 CheckLayersBehind = TRUE;
                 L = targetlayer;

                 FreeMem(am, sizeof(struct DeferedActionMessage));
            break; }

            case AMCODE_MOVEWINDOWINFRONTOF: { 
                 /* If GZZ window then also move outer window */
                 if (0 != (targetwindow->Flags & WFLG_GIMMEZEROZERO))
                 {
                   MoveLayerInFrontOf(targetwindow->BorderRPort->Layer,
                                      am->BehindWindow->WLayer);
                   RefreshWindowFrame(targetwindow);
                 }
                 MoveLayerInFrontOf(     targetwindow->WLayer,
                                    am->BehindWindow->WLayer);

                 CheckLayersBehind = TRUE;
                 //CheckLayersInFront = TRUE;
                 L = targetlayer;

		 NotifyDepthArrangement(targetwindow, IntuitionBase);
                 FreeMem(am, sizeof(struct DeferedActionMessage));
            break; }

            case AMCODE_SIZEWINDOW: {
		 /* correct dx, dy if necessary */

		 if ((targetwindow->LeftEdge + targetwindow->Width + am->dx) > targetwindow->WScreen->Width)
		 {
		    am->dx = targetwindow->WScreen->Width -
			     targetwindow->Width -
			     targetwindow->LeftEdge;
		 }
		 if ((targetwindow->TopEdge + targetwindow->Height + am->dy) > targetwindow->WScreen->Height)
		 {
		    am->dy = targetwindow->WScreen->Height -
			     targetwindow->Height -
			     targetwindow->TopEdge;
		 }

                 /* First erase the old frame on the right side and 
                    on the lower side if necessary, but only do this
                    for non-GZZ windows 
                 */

                 if (0 == (targetwindow->Flags & WFLG_GIMMEZEROZERO))
                 {
                   struct RastPort * rp = targetwindow->BorderRPort;
                   struct Layer * L = rp->Layer;
                   struct Rectangle rect;
                   struct Region * oldclipregion;
                   WORD ScrollX;
                   WORD ScrollY;
                   /* 
                   ** In case a clip region is installed then I have to 
                   ** install the regular cliprects of the layer
                   ** first. Otherwise the frame might not get cleared correctly.
                   */
                   LockLayer(0, L);

                   oldclipregion = InstallClipRegion(L, NULL);

		   ScrollX = L->Scroll_X;
                   ScrollY = L->Scroll_Y;

		   L->Scroll_X = 0;
                   L->Scroll_Y = 0;

                   if ((am->dy > 0) && (targetwindow->BorderBottom > 0))

                   {
		     rect.MinX = targetwindow->BorderLeft;
		     rect.MinY = targetwindow->Height - targetwindow->BorderBottom;
		     rect.MaxX = targetwindow->Width - 1;
		     rect.MaxY = targetwindow->Height - 1;

                     EraseRect(rp, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);

		     if (0 != (L->Flags & LAYERSIMPLE))
		     {
			 OrRectRegion(L->DamageList, &rect);
		     }
                   }

                   if ((am->dx > 0) && (targetwindow->BorderRight > 0))
                   {
		     rect.MinX = targetwindow->Width - targetwindow->BorderRight;
		     rect.MinY = targetwindow->BorderTop;
		     rect.MaxX = targetwindow->Width - 1;
		     rect.MaxY = targetwindow->Height - targetwindow->BorderBottom;

                     EraseRect(rp, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);

		     if (0 != (L->Flags & LAYERSIMPLE))
		     {
			 OrRectRegion(L->DamageList, &rect);
		     }
                   }

                   /*
                   ** Reinstall the clipregions rectangles if there are any.
                   */
                   if (NULL != oldclipregion)
                   {
                     InstallClipRegion(L, oldclipregion);
                   }

		   L->Scroll_X = ScrollX;
                   L->Scroll_Y = ScrollY;

		   UnlockLayer(L);
                 }

		 /* Before resizing the layers eraserect the area of all
		    GFLG_REL*** gadgets (except those in the window border */
	         EraseRelGadgetArea(targetwindow, IntuitionBase);

                 ((struct IntWindow *)targetwindow)->ZipWidth  = targetwindow->Width;
                 ((struct IntWindow *)targetwindow)->ZipHeight = targetwindow->Height;

                 targetwindow->Width += am->dx;
                 targetwindow->Height+= am->dy;
		 
                 /* I first resize the outer window if a GZZ window */
                 if (0 != (targetwindow->Flags & WFLG_GIMMEZEROZERO))
                 {
                   SizeLayer(NULL,
                             targetwindow->BorderRPort->Layer,
                             am->dx,
                             am->dy);
                 }

                 SizeLayer(NULL, 
                           targetlayer,
                           am->dx,
                           am->dy);
		 
                 /* 
                    Only if the window is smaller now there can be damage
                    to report to layers further behind.
                 */
                 if (am->dx < 0 || am->dy < 0)
                 {
                   CheckLayersBehind = TRUE;
                   if (0 == (targetwindow->Flags & WFLG_GIMMEZEROZERO))
                   {
                     L = targetlayer;
                   }
                   else
                   {
                     L = targetlayer->back;
                   }
                 }
		 /* Send GM_LAYOUT to all GA_RelS???? BOOPSI gadgets */

		 DoGMLayout(targetwindow->FirstGadget, w, NULL, -1, FALSE, IntuitionBase);

                 /* and redraw the window frame */
		 RefreshWindowFrame(targetwindow);

		 /* and refresh all gadgets except border gadgets */
		 int_refreshglist(w->FirstGadget, w, NULL, -1, 0, REFRESHGAD_BORDER, IntuitionBase);

		 if (targetwindow->IDCMPFlags & IDCMP_NEWSIZE)
                 {
		    /* Send IDCMP_NEWSIZE to resized window */
		    struct IntuiMessage *imsg;
		    imsg = alloc_intuimessage(IntuitionBase);
		    if (!imsg)
		    {
			/* Ouch, we're in BIG trouble */
			Alert(AT_DeadEnd|AN_Intuition|AG_NoMemory);
		    }
		    imsg->Class = IDCMP_NEWSIZE;

		    send_intuimessage(imsg, targetwindow, IntuitionBase);
                 }

                 FreeMem(am, sizeof(struct DeferedActionMessage));
            break; }

            case AMCODE_ZIPWINDOW: {
                 struct IntWindow * w = (struct IntWindow *)targetwindow;
                 WORD OldLeftEdge  = targetwindow->LeftEdge;
                 WORD OldTopEdge   = targetwindow->TopEdge;
                 WORD OldWidth     = targetwindow->Width;
                 WORD OldHeight    = targetwindow->Height;
                 WORD NewLeftEdge, NewTopEdge, NewWidth, NewHeight;

		 NewLeftEdge = OldLeftEdge;
		 if (w->ZipLeftEdge != ~0) NewLeftEdge = w->ZipLeftEdge;

		 NewTopEdge = OldTopEdge;
		 if (w->ZipTopEdge != ~0) NewTopEdge = w->ZipTopEdge;

		 NewWidth = OldWidth;
		 if (w->ZipWidth != ~0) NewWidth = w->ZipWidth;

		 NewHeight = OldHeight;
		 if (w->ZipHeight != ~0) NewHeight = w->ZipHeight;

                 /* correct new window coords if necessary */

		 if (NewWidth > targetwindow->WScreen->Width)
		     NewWidth = targetwindow->WScreen->Width;

		 if (NewHeight > targetwindow->WScreen->Height)
		     NewHeight = targetwindow->WScreen->Height;

		 if ((NewLeftEdge + NewWidth) > targetwindow->WScreen->Width)
		     NewLeftEdge = targetwindow->WScreen->Width - NewWidth;

		 if ((NewTopEdge + NewHeight) > targetwindow->WScreen->Height)
		     NewTopEdge = targetwindow->WScreen->Height - NewHeight;

		 /* First erase the old frame on the right side and 
        	    on the lower side if necessary, but only do this
        	    for non-GZZ windows 
		 */

		 if (0 == (targetwindow->Flags & WFLG_GIMMEZEROZERO))
		 {
		   struct RastPort * rp = targetwindow->BorderRPort;
		   struct Layer * L = rp->Layer;
		   struct Rectangle rect;
		   struct Region * oldclipregion;
		   WORD ScrollX;
		   WORD ScrollY;
		   WORD dx = NewWidth - OldWidth;
		   WORD dy = NewHeight - OldHeight;
		   /* 
		   ** In case a clip region is installed then I have to 
		   ** install the regular cliprects of the layer
		   ** first. Otherwise the frame might not get cleared correctly.
		   */
		   LockLayer(0, L);

		   oldclipregion = InstallClipRegion(L, NULL);

		   ScrollX = L->Scroll_X;
		   ScrollY = L->Scroll_Y;

		   L->Scroll_X = 0;
		   L->Scroll_Y = 0;

		   if ((dy > 0) && (targetwindow->BorderBottom > 0))

		   {
		     rect.MinX = targetwindow->BorderLeft;
		     rect.MinY = targetwindow->Height - targetwindow->BorderBottom;
		     rect.MaxX = targetwindow->Width - 1;
		     rect.MaxY = targetwindow->Height - 1;

        	     EraseRect(rp, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);

		     if (0 != (L->Flags & LAYERSIMPLE))
		     {
			 OrRectRegion(L->DamageList, &rect);
		     }
		   }

		   if ((dx > 0) && (targetwindow->BorderRight > 0))
		   {
		     rect.MinX = targetwindow->Width - targetwindow->BorderRight;
		     rect.MinY = targetwindow->BorderTop;
		     rect.MaxX = targetwindow->Width - 1;
		     rect.MaxY = targetwindow->Height - targetwindow->BorderBottom;

        	     EraseRect(rp, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);

		     if (0 != (L->Flags & LAYERSIMPLE))
		     {
			 OrRectRegion(L->DamageList, &rect);
		     }
		   }

		   /*
		   ** Reinstall the clipregions rectangles if there are any.
		   */
		   if (NULL != oldclipregion)
		   {
        	     InstallClipRegion(L, oldclipregion);
		   }

		   L->Scroll_X = ScrollX;
		   L->Scroll_Y = ScrollY;

		   UnlockLayer(L);
		 }

		 if ((NewWidth != OldWidth) || (NewHeight != OldHeight))
		 {
		     /* Before resizing the layers eraserect the area of all
			GFLG_REL*** gadgets (except those in the window border */
		     EraseRelGadgetArea(targetwindow, IntuitionBase);
		 }

                 targetwindow->LeftEdge = NewLeftEdge;
                 targetwindow->TopEdge  = NewTopEdge;
                 targetwindow->Width    = NewWidth;
                 targetwindow->Height   = NewHeight; 
		 
                 /* check for GZZ window */
                 if (0 != (targetwindow->Flags & WFLG_GIMMEZEROZERO))
                 {
                   /* move outer window first */
                   MoveSizeLayer(targetwindow->BorderRPort->Layer,
                                 NewLeftEdge - OldLeftEdge,
                                 NewTopEdge  - OldTopEdge,
                                 NewWidth    - OldWidth,
                                 NewHeight   - OldHeight);
                 }

                 L = targetlayer;
                 CheckLayersBehind = TRUE;

                 MoveSizeLayer(targetlayer,
                               NewLeftEdge - OldLeftEdge,
                               NewTopEdge  - OldTopEdge,
                               NewWidth    - OldWidth,
                               NewHeight   - OldHeight);

                 if (w->ZipLeftEdge != ~0) w->ZipLeftEdge = OldLeftEdge;
                 if (w->ZipTopEdge  != ~0) w->ZipTopEdge  = OldTopEdge;
                 if (w->ZipWidth  != ~0) w->ZipWidth    = OldWidth;
                 if (w->ZipHeight != ~0) w->ZipHeight   = OldHeight;

		 /* Change width of dragbar gadget */

		 /* Send GM_LAYOUT to all GA_Rel??? BOOPSI gadgets */
		 DoGMLayout(targetwindow->FirstGadget, targetwindow, NULL, -1, FALSE, IntuitionBase);

		 /* and redraw the window frame */
		 RefreshWindowFrame(targetwindow);
		 
		 /* and refresh all gadgets except border gadgets */
		 int_refreshglist(targetwindow->FirstGadget, targetwindow, NULL, -1, 0, REFRESHGAD_BORDER, IntuitionBase);

		 if (targetwindow->IDCMPFlags & IDCMP_CHANGEWINDOW)
                 {
		    /* Send IDCMP_CHANGEWINDOW to resized window */
		    struct IntuiMessage *imsg;
		    imsg = alloc_intuimessage(IntuitionBase);
		    if (!imsg)
		    {
			/* Ouch, we're in BIG trouble */
			Alert(AT_DeadEnd|AN_Intuition|AG_NoMemory);
		    }
		    imsg->Class = IDCMP_CHANGEWINDOW;
		    imsg->Code = CWCODE_MOVESIZE;
		    send_intuimessage(imsg, targetwindow, IntuitionBase);
                 }

                 FreeMem(am, sizeof(struct DeferedActionMessage));
            break; }

	    case AMCODE_CHANGEWINDOWBOX: {

                 /*
                  * For non GZZ windows delete the right and lower part
                  * of the frame IF the window is getting wider or
                  * higher
                  */
                 if (0 == (targetwindow->Flags & WFLG_GIMMEZEROZERO))
                 {		 
                   if (am->height > targetwindow->Height)
                   {
                     struct RastPort * rp = targetwindow->BorderRPort;
                     SetAPen(rp, 0);
                     Move(rp, 0, targetwindow->Height-targetwindow->BorderBottom);
                     Draw(rp, targetwindow->Width-targetwindow->BorderRight, targetwindow->Height-targetwindow->BorderBottom);
                   }
                   if (am->width > targetwindow->Width)
                   {
                     struct RastPort * rp = targetwindow->BorderRPort;
                     SetAPen(rp, 0);
                     Move(rp, targetwindow->Width-targetwindow->BorderRight, 0);
                     Draw(rp, targetwindow->Width-targetwindow->BorderRight, targetwindow->Height-targetwindow->BorderBottom);
                   }
                 }

                 /* Now try to move and resize the window */
 		 if (FALSE == intui_ChangeWindowBox(targetwindow
		     	                          , am->left, am->top
			                          , am->width, am->height))
	         {
	           RefreshWindowFrame(targetwindow);
		   break;
		 }

                 ((struct IntWindow *)targetwindow)->ZipLeftEdge = targetwindow->LeftEdge; 
                 ((struct IntWindow *)targetwindow)->ZipTopEdge  = targetwindow->TopEdge;
                 ((struct IntWindow *)targetwindow)->ZipWidth  = targetwindow->Width;
                 ((struct IntWindow *)targetwindow)->ZipHeight = targetwindow->Height;


		 /* Change width of dragbar gadget */


		 /* Send GM_LAYOUT to all GA_RelSpecial BOOPSI gadgets */
		 DoGMLayout(targetwindow->FirstGadget, w, NULL, -1, FALSE, IntuitionBase);

		 if (targetwindow->IDCMPFlags & IDCMP_CHANGEWINDOW)
                 {
		    /* Send IDCMP_CHANGEWINDOW to resized window */
		    struct IntuiMessage *imsg;
		    imsg = alloc_intuimessage(IntuitionBase);
		    if (!imsg)
		    {
			/* Ouch, we're in BIG trouble */
			Alert(AT_DeadEnd|AN_Intuition|AG_NoMemory);
		    }
		    imsg->Class = IDCMP_CHANGEWINDOW;

		    send_intuimessage(imsg, targetwindow, IntuitionBase);
                 }
		FreeMem(am, sizeof (struct DeferedActionMessage));

                CheckLayersBehind = TRUE;
                L = targetlayer;

		break; }

	}

	if (TRUE == CheckLayersBehind)
	{
	  /* Walk through all layers behind including the layer L
	     and check whether a layer needs a refresh 
	  */ 
	  struct Layer * _L = L;
	  struct Window * _W;

	  while (NULL != _L)
	  {
	    /* Does this Layer need a refresh and does it belong
	       to a Window ?? */
	    if (0 != (_L->Flags & LAYERREFRESH) &&
	        NULL != _L->Window)
	    {
	      /* Does it belong to a GZZ window and is it
	         the outer window of that GZZ window? */
	      if (0  != (((struct Window *)_L->Window)->Flags & WFLG_GIMMEZEROZERO) &&
	          _L ==  ((struct Window *)_L->Window)->BorderRPort->Layer            )
	      {
	        /* simply refresh that window's frame */

		BeginUpdate(_L);
	        RefreshWindowFrame((struct Window *)_L->Window);
		EndUpdate(_L, TRUE);

	        _L->Flags &= ~LAYERREFRESH;
	      }
	      else
	        windowneedsrefresh((struct Window *)_L->Window,
	                           IntuitionBase);
	    }
	   _L = _L->back;

	  } /* while (NULL != _L) */

	} /* if (TRUE == CheckLayersBehind) */

	if (TRUE == CheckLayersInFront)
	{
	  /* Walk through all layers in front of including the layer L
	     and check whether a layer needs a refresh 
	  */
	  struct Window *_W;

	  if (TRUE == CheckLayersBehind)
	    L=L->front; /* the layer L has already been checked */

	  while (NULL != L)
	  {  
	    /* Does this Layer need a refresh and does it belong
	       to a Window ?? */
	    if (0 != (L->Flags & LAYERREFRESH) &&
	        NULL != L->Window)
	    {
	      /* Does it belong to a GZZ window and is it
	         the outer window of that GZZ window? */
	      if (0  != (((struct Window *)L->Window)->Flags & WFLG_GIMMEZEROZERO) &&
	          L  ==  ((struct Window *)L->Window)->BorderRPort->Layer            )
	      {
	        /* simply refresh that window's frame */

		BeginUpdate(L);
	        RefreshWindowFrame((struct Window *)L->Window);
		EndUpdate(L, TRUE);

	        L->Flags &= ~LAYERREFRESH;
	      }
	      else
	        windowneedsrefresh((struct Window *)L->Window,
	                           IntuitionBase);
	    }

	    L = L->front;

	  } /* while (NULL != L) */

	} /* if (TRUE == CheckLayersInFront) */

    } /* while ((am = (struct DeferedActionMessage *)GetMsg (iihdata->DeferedActionPort))) */

    D(bug("Outside pollingloop\n"));
    return (oldchain);
}


inline VOID send_intuimessage(struct IntuiMessage *imsg,
			      struct Window *w,
			      struct IntuitionBase *IntuitionBase)
{
    /* Mark the message as taken */    

    /* Reply the message to intuition */
    imsg->ExecMessage.mn_ReplyPort = w->WindowPort;

    imsg->IDCMPWindow = w;
    
    PutMsg(w->UserPort, (struct Message *)imsg);
}

inline VOID free_intuimessage(struct IntuiMessage *imsg,
			      struct IntuitionBase *IntuitionBase)
{
    FreeMem(imsg, sizeof (struct ExtIntuiMessage));
}

inline struct IntuiMessage *alloc_intuimessage(struct IntuitionBase *IntuitionBase)
{
    struct IntuiMessage	*imsg;
    
    imsg = AllocMem(sizeof(struct ExtIntuiMessage), MEMF_CLEAR|MEMF_PUBLIC);
    if (imsg)
    {
    	CurrentTime(&imsg->Seconds, &imsg->Micros);
    }
    
    return imsg;
}
