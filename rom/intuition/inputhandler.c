#define AROS_ALMOST_COMPATIBLE 1 /* NEWLIST macro */
#include <proto/exec.h>
#include <proto/boopsi.h>
#include <proto/intuition.h>
#include <proto/alib.h>
#include <proto/layers.h>
#include <exec/memory.h>
#include <exec/alerts.h>
#include <exec/interrupts.h>
#include <exec/ports.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <intuition/sghooks.h>
#include "inputhandler.h"

#include "boopsigadgets.h"
#include "boolgadgets.h"
#include "propgadgets.h"
#include "strgadgets.h"
#include "intuition_intern.h" /* EWFLG_xxx */

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

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
	    port = AllocMem(sizeof (struct MsgPort), MEMF_PUBLIC|MEMF_CLEAR);
	    if (port)
	    {
	        ULONG lock;
		
	    	port->mp_Flags   = PA_SIGNAL;
	    	port->mp_SigBit  = SIGB_INTUITION;
	    	port->mp_SigTask = FindTask("input.device");
	    	NEWLIST( &(port->mp_MsgList) );
	    	iihdata->IntuiReplyPort = port;
	    	
		iihandler->is_Code = (APTR)AROS_ASMSYMNAME(IntuiInputHandler);
		iihandler->is_Data = iihdata;
		iihandler->is_Node.ln_Pri	= 50;
		iihandler->is_Node.ln_Name	= "Intuition InputHandler";
		
		lock = LockIBase(0UL);

		iihdata->IntuitionBase = IntuitionBase;
		
		UnlockIBase(lock);
		
		GetPrivIBase(IntuitionBase)->IntuiReplyPort = iihdata->IntuiReplyPort;

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
    */
    FreeMem(((struct IIHData *)iihandler->is_Data)->IntuiReplyPort, sizeof (struct MsgPort));
    
    
    FreeMem(iihandler->is_Data, sizeof (struct IIHData));
    FreeMem(iihandler, sizeof (struct Interrupt));

    return;
}


#define ADDREL(gad,flag,w,field) ((gad->Flags & (flag)) ?  w->field : 0)

#define GetLeft(gad,w)           (ADDREL(gad,GFLG_RELRIGHT ,w,Width)  + w->LeftEdge + gad->LeftEdge)
#define GetTop(gad,w)            (ADDREL(gad,GFLG_RELBOTTOM,w,Height) + w->TopEdge  + gad->TopEdge)
#define GetWidth(gad,w)          (ADDREL(gad,GFLG_RELWIDTH ,w,Width)  + gad->Width)
#define GetHeight(gad,w)         (ADDREL(gad,GFLG_RELHEIGHT,w,Height) + gad->Height)

#define InsideGadget(w,gad,x,y)   \
	    ((x) >= GetLeft(gad,w) && (y) >= GetTop(gad,w) \
	     && (x) < GetLeft(gad,w) + GetWidth(gad,w) \
	     && (y) < GetTop(gad,w) + GetHeight(gad,w))

/*****************
**  FindGadget	**
*****************/
struct Gadget * FindGadget (struct Window * window, int x, int y,
			struct GadgetInfo * gi)
{
    struct Gadget * gadget;
    struct gpHitTest gpht;
    int gx, gy;

    gpht.MethodID     = GM_HITTEST;
    gpht.gpht_GInfo   = gi;

    for (gadget=window->FirstGadget; gadget; gadget=gadget->NextGadget)
    {
	if ((gadget->GadgetType & GTYP_GTYPEMASK) != GTYP_CUSTOMGADGET)
	{
	    /* Mouseclick inside the gadget? */
	    gx = x - GetLeft(gadget,window);
	    gy = y - GetTop(gadget,window);

	    if (gx >= 0
		&& gy >= 0
		&& gx < GetWidth(gadget,window)
		&& gy < GetHeight(gadget,window)
	    )
		break;
	}
	else
	{
	    /* Get coords relative to window */
    	    gpht.gpht_Mouse.X = x - window->LeftEdge;
    	    gpht.gpht_Mouse.Y = y - window->TopEdge;
	    
	    if (DoMethodA ((Object *)gadget, (Msg)&gpht) == GMR_GADGETHIT)
		break;
	}
    }

    return (gadget);
} /* FindGadget */




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
    struct Screen	* screen;
    struct Gadget *gadget = iihdata->ActiveGadget;
    struct IntuitionBase *IntuitionBase = iihdata->IntuitionBase;
    ULONG  lock;
    char *ptr = NULL;
    WORD mpos_x = iihdata->LastMouseX, mpos_y = iihdata->LastMouseY;
    struct GadgetInfo stackgi, *gi = &stackgi;
    BOOL reuse_event = FALSE;
    struct Window *w;
    
    D(bug("Inside intuition inputhandler, active window=%p\n", w));

    for (ie = oldchain; ie; ie = ((reuse_event) ? ie : ie->ie_NextEvent))
    {
    
	struct Window *new_w;
	BOOL swallow_event = FALSE;
	BOOL new_active_window = FALSE;
    
    	D(bug("iih: Handling event of class %d, code %d\n", ie->ie_Class, ie->ie_Code));
	reuse_event = FALSE;
	ptr = NULL;

    /* If there is no active window, and this is not a SELECTDOWN,
       mouse event, then exit because we do not have a window
       to send events to. */
       
       /* Use event to find the active window */
       
       
        lock = LockIBase(0UL);
       
    	w = IntuitionBase->ActiveWindow;
	new_w = intui_FindActiveWindow(ie, &swallow_event, IntuitionBase);
	
	D(bug("iih:New active window: %p\n", new_w));

	if (new_w)
	{
	    if ( new_w != w )
	    {

		D(bug("Activating new window (title %s)\n", new_w->Title));
		
		D(bug("Window activated\n"));
		w = new_w;
		new_active_window = TRUE;
	    }
	}
	
	

        if (NULL == w)
	{
	    /* We can't have an active gadget if we don't have an active window */
	    iihdata->ActiveGadget = NULL;
	    gadget = NULL;
	    UnlockIBase(lock);
	    continue;
	}
	    
	UnlockIBase(lock);
	
	/* At this point w points to a valid active window */
	
	if (new_active_window)
	{
	    int_activatewindow(w, IntuitionBase);
	}
	
	if (swallow_event)
	    continue;
		     
	
	/* If the last InputEvent was swallowed, we can reuse the IntuiMessage.
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

	im->Class	= 0L;
	im->IAddress	= NULL;
	im->MouseX	= mpos_x;
	im->MouseY	= mpos_y;
	im->IDCMPWindow = w;
	    
	    
	screen = w->WScreen;

	gi->gi_Screen	  = screen;
	gi->gi_Window	  = w;
	gi->gi_Domain	  = *((struct IBox *)&w->LeftEdge);
	gi->gi_RastPort   = w->RPort;
	gi->gi_Pens.DetailPen = gi->gi_Screen->DetailPen;
	gi->gi_Pens.BlockPen  = gi->gi_Screen->BlockPen;
	gi->gi_DrInfo	  = &(((struct IntScreen *)screen)->DInfo);

	switch (ie->ie_Class)
	{
	    
	case IECLASS_REFRESHWINDOW:
	    ptr       = "REFRESHWINDOW";
	    im->Class = IDCMP_REFRESHWINDOW;

	    RefreshGadgets (w->FirstGadget, w, NULL);
	    break;

	case IECLASS_SIZEWINDOW:
	    ptr       = "NEWSIZE";
	    im->Class = IDCMP_NEWSIZE;

	    /* Send GM_LAYOUT to all GA_RelSpecial BOOPSI gadgets */
	    DoGMLayout(w->FirstGadget, w, NULL, -1, FALSE, IntuitionBase);
	    break;

	case IECLASS_RAWMOUSE:
	    /* IECLASS_RAWMOUSE events are let through even when there is no active window */
	    if (im)
	    {
		im->Code	= ie->ie_Code;
	    }
	    

	    ptr = "RAWMOUSE";

	    switch (ie->ie_Code)
	    {
	    case SELECTDOWN: {
		BOOL new_gadget = FALSE;


 	        /* 
	        **  The mouse coordinates relative to the upper left
	        **  corner of the window
	        */
	        im->MouseX	= ie->ie_X - w->LeftEdge;
	        im->MouseY	= ie->ie_Y - w->TopEdge;

		im->Class = IDCMP_MOUSEBUTTONS;
		ptr = "MOUSEBUTTONS";

		if (!gadget)
		{

  
		    gadget = FindGadget (w, ie->ie_X, ie->ie_Y, gi);
		    if (gadget)
		    {
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
			HandlePropSelectDown(gadget, w, NULL, im->MouseX, im->MouseY, IntuitionBase);
			break;

		    case GTYP_STRGADGET:
			/* If the click was inside the active strgad,
			** then let it update cursor pos,
			** else deactivate stringadget and reuse event.
			*/

			if (InsideGadget(w, gadget, ie->ie_X, ie->ie_Y))
			{
			    UWORD imsgcode;

			    HandleStrInput(gadget, gi, ie, &imsgcode, IntuitionBase);
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

			gpi.MethodID	= ((new_gadget) ? GM_GOACTIVE : GM_HANDLEINPUT);
			gpi.gpi_GInfo	= gi;
			gpi.gpi_IEvent	= ie;
			gpi.gpi_Termination = &termination;
			gpi.gpi_Mouse.X = im->MouseX;
			gpi.gpi_Mouse.Y = im->MouseY;
			gpi.gpi_TabletData	= NULL;

			retval = DoMethodA ((Object *)gadget, (Msg)&gpi);

			if (retval != GMR_MEACTIVE)
			{
			    struct gpGoInactive gpgi;

			    if (retval & GMR_REUSE)
				reuse_event = TRUE;

			    if (retval & GMR_VERIFY)
			    {
				im->Class = IDCMP_GADGETUP;
				im->IAddress = gadget;
				ptr	 = "GADGETUP";
				im->Code = termination & 0x0000FFFF;
			    }
			    else
			    {
				im->Class = 0; /* Swallow event */
			    }

			    gpgi.MethodID = GM_GOINACTIVE;
			    gpgi.gpgi_GInfo = gi;
			    gpgi.gpgi_Abort = 0;

			    DoMethodA((Object *)gadget, (Msg)&gpgi);

			    gadget = NULL;
			}

			break; }


		    } /* switch (GadgetType) */

		} /* if (a gadget is active) */

		if (im->Class == IDCMP_MOUSEBUTTONS)
		    ptr = "MOUSEBUTTONS";

		}break; /* SELECTDOWN */

	    case SELECTUP:
		im->Class = IDCMP_MOUSEBUTTONS;
		ptr = "MOUSEBUTTONS";

 	        /* 
	        **  The mouse coordinates relative to the upper left
	        **  corner of the window
	        */
	        im->MouseX	= ie->ie_X - w->LeftEdge;
	        im->MouseY	= ie->ie_Y - w->TopEdge;


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
			gpi.gpi_Mouse.X = im->MouseX;
			gpi.gpi_Mouse.Y = im->MouseY;
			gpi.gpi_TabletData	= NULL;

			retval = DoMethodA ((Object *)gadget, (Msg)&gpi);


			if (retval != GMR_MEACTIVE)
			{
			    struct gpGoInactive gpgi;

			    if (retval & GMR_REUSE)
				reuse_event = TRUE;

			    if (    (retval & GMR_VERIFY)
				 && (gadget->Activation & GACT_RELVERIFY))
			    {
				im->Class = IDCMP_GADGETUP;
				im->IAddress = gadget;
				ptr	 = "GADGETUP";
				im->Code = termination & 0x0000FFFF;
			    }
			    else
			    {
				im->Class = 0; /* Swallow event */
			    }

			    gpgi.MethodID = GM_GOINACTIVE;
			    gpgi.gpgi_GInfo = gi;
			    gpgi.gpgi_Abort = 0;

			    DoMethodA((Object *)gadget, (Msg)&gpgi);

			    gadget = NULL;
			}

			break; }

		    } /* switch GadgetType */

		} /* if (a gadget is currently active) */



		break; /* SELECTUP */

	    case MENUDOWN:
		im->Class = IDCMP_MOUSEBUTTONS;
		ptr = "MOUSEBUTTONS";

 	        /* 
	        **  The mouse coordinates relative to the upper left
	        **  corner of the window
	        */
	        im->MouseX	= ie->ie_X - w->LeftEdge;
	        im->MouseY	= ie->ie_Y - w->TopEdge;

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
			gpi.gpi_Mouse.X     = im->MouseX;
			gpi.gpi_Mouse.Y     = im->MouseY;
			gpi.gpi_TabletData  = NULL;

			retval = DoMethodA((Object *)gadget, (Msg)&gpi);

			if (retval != GMR_MEACTIVE)
			{
			    struct gpGoInactive gpgi;

			    if (retval & GMR_REUSE)
				reuse_event = TRUE;

			    if (    (retval & GMR_VERIFY)
				 && (gadget->Activation & GACT_RELVERIFY))
			    {
				im->Class = IDCMP_GADGETUP;
				im->IAddress = gadget;
				ptr	 = "GADGETUP";
				im->Code = termination & 0x0000FFFF;
			    }
			    else
			    {
				im->Class = 0; /* Swallow event */
			    }

			    gpgi.MethodID = GM_GOINACTIVE;
			    gpgi.gpgi_GInfo = gi;
			    gpgi.gpgi_Abort = 0;

			    DoMethodA((Object *)gadget, (Msg)&gpgi);

			    gadget = NULL;

			} /* if (retval != GMR_MEACTIVE) */

		    } /* if (active gadget is a BOOPSI gad) */

		} /* if (there is an active gadget) */
		break; /* MENUDOWN */

	    case MENUUP:
		im->Class = IDCMP_MOUSEBUTTONS;
		ptr = "MOUSEBUTTONS";

 	        /* 
	        **  The mouse coordinates relative to the upper left
	        **  corner of the window
	        */
	        im->MouseX	= ie->ie_X - w->LeftEdge;
	        im->MouseY	= ie->ie_Y - w->TopEdge;

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
			gpi.gpi_Mouse.X     = im->MouseX;
			gpi.gpi_Mouse.Y     = im->MouseY;
			gpi.gpi_TabletData  = NULL;

			retval = DoMethodA((Object *)gadget, (Msg)&gpi);

			if (retval != GMR_MEACTIVE)
			{
			    struct gpGoInactive gpgi;

			    if (retval & GMR_REUSE)
				reuse_event = TRUE;

			    if (    (retval & GMR_VERIFY)
				 && (gadget->Activation & GACT_RELVERIFY))
			    {
				im->Class = IDCMP_GADGETUP;
				im->IAddress = gadget;
				ptr	 = "GADGETUP";
				im->Code = termination & 0x0000FFFF;
			    }
			    else
			    {
				im->Class = 0; /* Swallow event */
			    }

			    gpgi.MethodID = GM_GOINACTIVE;
			    gpgi.gpgi_GInfo = gi;
			    gpgi.gpgi_Abort = 0;

			    DoMethodA((Object *)gadget, (Msg)&gpgi);


			    gadget = NULL;
			} /* if (retval != GMR_MEACTIVE) */

		    } /* if (active gadget is a BOOPSI gad) */

		} /* if (there is an active gadget) */

		break; /* MENUUP */


	    case IECODE_NOBUTTON: { /* MOUSEMOVE */
		struct IntuiMessage *msg, *succ;

		im->Class = IDCMP_MOUSEMOVE;
 	        /* 
	        **  The mouse coordinates relative to the upper left
	        **  corner of the window
	        */
	        im->MouseX	= ie->ie_X - w->LeftEdge;
	        im->MouseY	= ie->ie_Y - w->TopEdge;

		ptr = "MOUSEMOVE";
		iihdata->LastMouseX = ie->ie_X;
		iihdata->LastMouseY = ie->ie_Y;
		
		/* Set the screens mouse coords.
		   This won't work if no window is active */
		w->WScreen->MouseX = ie->ie_X;
		w->WScreen->MouseY = ie->ie_Y;



		if (gadget)
		{
		    int inside = InsideGadget(w,gadget,im->MouseX, im->MouseY);
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
			HandlePropMouseMove(gadget
				,w
				,NULL
				/* Delta movement */
				,ie->ie_X - mpos_x
				,ie->ie_Y - mpos_y
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
			gpi.gpi_Mouse.X     = im->MouseX;
			gpi.gpi_Mouse.Y     = im->MouseY;
			gpi.gpi_TabletData  = NULL;

			retval = DoMethodA ((Object *)gadget, (Msg)&gpi);
			
			/* Hack to make Gadtools slider & scroller gadgets work */
			if (retval == GMR_INTERIMUPDATE)
			{
			    im->Code = termination & 0x0000FFFF;
			    im->IAddress = gadget;
			    retval = GMR_MEACTIVE;
			}

			if (retval != GMR_MEACTIVE)
			{
			    struct gpGoInactive gpgi;

			    if (retval & GMR_REUSE)
				reuse_event = TRUE;

			    if (    (retval & GMR_VERIFY)
				 && (gadget->Activation & GACT_RELVERIFY))
			    {
				im->Class = IDCMP_GADGETUP;
				im->IAddress = gadget;
				ptr	 = "GADGETUP";
				im->Code = termination & 0x0000FFFF;
			    }
			    else
			    {
				im->Class = 0; /* Swallow event */
			    }

			    gpgi.MethodID = GM_GOINACTIVE;
			    gpgi.gpgi_GInfo = gi;
			    gpgi.gpgi_Abort = 0;

			    DoMethodA((Object *)gadget, (Msg)&gpgi);

			    gadget = NULL;
			}



			break; }

		} /* switch GadgetType */
	    } /* if (a gadget is currently active) */

		/* Limit the number of IDCMP_MOUSEMOVE messages sent to intuition.
		   note that this comes after handling gadgets, because gadgets should get all events.
		*/

		if (w->UserPort)
		{
		    msg = (struct IntuiMessage *)w->UserPort->mp_MsgList.lh_Head;

		    Forbid ();

		    while ((succ = (struct IntuiMessage *)msg->ExecMessage.mn_Node.ln_Succ))
		    {
			if (msg->Class == IDCMP_MOUSEMOVE)
			{
#warning TODO: allow a number of such messages
			   break;
			}

			msg = succ;
		    }
		    Permit ();
		}

		/* If there is, don't add another one */
		if (succ)
		    continue;
		    

	    break; }



	    } /* switch (im->im_Code)  (what button was pressed ?) */
	    break;

		/* Check if there is already a MOUSEMOVE in the msg queue
		** of the task
		*/




	case IECLASS_RAWKEY:
	    im->Class	    = IDCMP_RAWKEY;
	    im->Code	    = ie->ie_Code;
	    im->Qualifier   = ie->ie_Qualifier;

	    if (!(ie->ie_Code & 0x8000))
	    {
		ptr = "RAWKEY PRESSED";


		if (gadget)
		{

		    switch (gadget->GadgetType & GTYP_GTYPEMASK)
		    {
		    case GTYP_STRGADGET: {
			UWORD imsgcode;
			ULONG ret = HandleStrInput(gadget, gi, ie, &imsgcode, IntuitionBase);
			if (ret == SGA_END)
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
			gpi.gpi_Mouse.X     = im->MouseX;
			gpi.gpi_Mouse.Y     = im->MouseY;
			gpi.gpi_TabletData  = NULL;

			retval = DoMethodA((Object *)gadget, (Msg)&gpi);

			if (retval != GMR_MEACTIVE)
			{
			    struct gpGoInactive gpgi;

			    if (retval & GMR_REUSE)
				reuse_event = TRUE;

			    if (    (retval & GMR_VERIFY)
				 && (gadget->Activation & GACT_RELVERIFY))
			    {
				im->Class = IDCMP_GADGETUP;
				im->IAddress = gadget;
				ptr	 = "GADGETUP";
				im->Code = termination & 0x0000FFFF;
			    }
			    else
			    {
				im->Class = 0; /* Swallow event */
			    }

			    gpgi.MethodID = GM_GOINACTIVE;
			    gpgi.gpgi_GInfo = gi;
			    gpgi.gpgi_Abort = 0;

			    DoMethodA((Object *)gadget, (Msg)&gpgi);

			    gadget = NULL;

			}


			break;}  /* case BOOPSI custom gadget type */

		    } /* switch (gadget type) */

		} /* if (a gadget is currently active) */
	    }
	    else /* key released */
	    {
		ptr = "RAWKEY RELEASED";
	    }
	    break; /* case IECLASS_RAWKEY */

	case IECLASS_ACTIVEWINDOW:
	    im->Class = IDCMP_ACTIVEWINDOW;
	    ptr = "ACTIVEWINDOW";
	    break;

	case IDCMP_INACTIVEWINDOW:
	    im->Class = IDCMP_INACTIVEWINDOW;
	    ptr = "INACTIVEWINDOW";
	    break;

	default:
	    ptr = NULL;
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

    D(bug("Poll the replyport for replies from apps\n"));
    
    

    /* Empty port */
    while ((im = (struct IntuiMessage *)GetMsg (iihdata->IntuiReplyPort)))
    {
    	if (IDCMP_WBENCHMESSAGE == im->Class)
	{
	    struct shortIntuiMessage * msg = (struct shortIntuiMessage *)im;
            BOOL CheckLayersBehind = FALSE;
            BOOL CheckLayersInFront = FALSE;
            struct Layer * L;
	    
	    switch (im->Code)
	    {
		case IMCODE_CLOSEWINDOW:
		    L = ((struct Window *)im->IAddress)->WLayer->back;
		    if (NULL != L)
		      CheckLayersBehind = TRUE;
		    int_closewindow((struct Window *)im->IAddress, IntuitionBase);
		break;
		 
		case IMCODE_WINDOWTOFRONT: {
		    L = msg->Window->WLayer;
		    if (0 == (L->Flags & LAYERBACKDROP))
		    {
		      UpfrontLayer(NULL, L);
		      
		      /* only this layer needs to be updated */
		      if (0 != (L->Flags & LAYERREFRESH))
		      {
		        struct IntuiMessage * IM = alloc_intuimessage(IntuitionBase);
		        L->Flags &= ~LAYERREFRESH;
		        if (NULL != IM)
		        {
		          IM->Class = IDCMP_REFRESHWINDOW;
		          send_intuimessage(IM, msg->Window, IntuitionBase);
		        }
		      }
		    } 
		    FreeMem(msg, sizeof(struct shortIntuiMessage));
		break; }

		case IMCODE_WINDOWTOBACK: {
		    /* I don't move backdrop layers! */
		    if (0 == (L->Flags & LAYERBACKDROP))
		    {
		      /* The layer behind the one to move will be the 
		         first one to check for damage */
		       
		      L = msg->Window->WLayer->back;
		      BehindLayer(NULL, L);
		      CheckLayersBehind = TRUE;
		    }
		    
		    FreeMem(msg, sizeof(struct shortIntuiMessage));
		break; }
		    
		case IMCODE_ACTIVATEWINDOW: {
		    int_activatewindow(msg->Window, IntuitionBase);
		    
		    FreeMem(msg, sizeof (struct shortIntuiMessage));
		break; }


                case IMCODE_MOVEWINDOW: { 
                     MoveLayer(NULL,
                               msg->Window->WLayer,
                               msg->dx,
                               msg->dy);

                     msg->Window->LeftEdge += msg->dx;
                     msg->Window->TopEdge  += msg->dy;

                     CheckLayersBehind = TRUE;
                     L = msg->Window->WLayer;
                    
                     FreeMem(msg, sizeof(struct shortIntuiMessage));
                break; }

                case IMCODE_MOVEWINDOWINFRONTOF: { 
                     MoveLayerInFrontOf(msg->      Window->WLayer,
                                        msg->BehindWindow->WLayer);
                     CheckLayersBehind = TRUE;
                     CheckLayersInFront = TRUE;
                     L = msg->Window->WLayer;
                    
                     FreeMem(msg, sizeof(struct shortIntuiMessage));
                break; }
                    
                case IMCODE_SIZEWINDOW: {
                     SizeLayer(NULL, 
                               msg->Window->WLayer,
                               msg->dx,
                               msg->dy);

                     msg->Window->Width += msg->dx;
                     msg->Window->Height+= msg->dy;
                     RefreshWindowFrame(w);

                     CheckLayersBehind = TRUE;
                     L = msg->Window->WLayer;
                     /* and redraw the window frame */
                     RefreshWindowFrame(msg->Window);

                     FreeMem(msg, sizeof(struct shortIntuiMessage));
                break; }
                         
                case IMCODE_ZIPWINDOW: {
                     struct IntWindow * w = (struct IntWindow *)msg->Window;
                     UWORD OldLeftEdge  = w->window.LeftEdge;
                     UWORD OldTopEdge   = w->window.TopEdge;
                     UWORD OldWidth     = w->window.Width;
                     UWORD OldHeight    = w->window.Height;
                    
                     MoveSizeLayer(w->window.WLayer,
                                   w->ZipLeftEdge - OldLeftEdge,
                                   w->ZipTopEdge  - OldTopEdge,
                                   w->ZipWidth    - OldWidth,
                                   w->ZipHeight   - OldHeight);

                     w->window.LeftEdge = w->ZipLeftEdge;
                     w->window.TopEdge  = w->ZipTopEdge;
                     w->window.Width    = w->ZipWidth;
                     w->window.Height   = w->ZipHeight; 
                    
                     w->ZipLeftEdge = OldLeftEdge;
                     w->ZipTopEdge  = OldTopEdge;
                     w->ZipWidth    = OldWidth;
                     w->ZipHeight   = OldHeight;
                    
                     CheckLayersBehind = TRUE;
                     L = msg->Window->WLayer;
                    
                     FreeMem(msg, sizeof(struct shortIntuiMessage));
                break; }
		
		/* ActivateWindow + other stuff goes here */
	    }
	    
	    if (TRUE == CheckLayersBehind)
	    {
	      /* Walk through all layers behind including the layer L
	         and check whether a layer needs a refresh 
	      */ 
	      struct Layer * _L = L;
	      while (NULL != _L)
	      {
	        if (0 != (_L->Flags & LAYERREFRESH) && NULL != _L->Window)
	          windowneedsrefresh((struct Window *)_L->Window,
	                             IntuitionBase);
	       
	       _L = _L->back;
	      }
	    }
	    
	    if (TRUE == CheckLayersInFront)
	    {
	      /* Walk through all layers in front of including the layer L
	         and check whether a layer needs a refresh 
	      */
	      if (TRUE == CheckLayersBehind)
	        L=L->front; /* the layer L has already been checked */
	      
	      while (NULL != L)
	      {  
	        if (0 != (L->Flags & LAYERREFRESH) && NULL != L->Window)
	          windowneedsrefresh((struct Window *)L->Window,
	                             IntuitionBase);
	      
	        L = L->front;
	      }
	    }
	}
	else
	{
	    free_intuimessage(im, IntuitionBase);
	}
    }

    D(bug("Outside pollingloop\n"));
    return (oldchain);
}

void windowneedsrefresh(struct Window * w, 
                        struct IntuitionBase * IntuitionBase )
{
  /* Supposed to send a message to this window, saying that it needs a
     refresh. I will check whether there is no such a message queued in
     its messageport, though. It only needs one such message! 
  */
  struct IntuiMessage * IM;
  BOOL found = FALSE;
  
  /* Can use Forbid() for this */
  
  Forbid();
  
  IM = (struct IntuiMessage *)w->UserPort->mp_MsgList.lh_Head;

  /* reset the flag in the layer */
  w->WLayer->Flags &= ~LAYERREFRESH;

  while ((NULL != IM) && !found)
  {
    /* Does the window already have such a message? */
    if (IDCMP_REFRESHWINDOW == IM->Class)
    {
kprintf("Window %s already has a refresh message pending!!\n",w->Title);
      found = TRUE;
    }
    IM = (struct IntuiMessage *)IM->ExecMessage.mn_Node.ln_Succ;
  }
  
  Permit();

kprintf("Sending a refresh message to window %s!!\n",w->Title);
  if (!found)
  {
    IM = alloc_intuimessage(IntuitionBase);
    if (NULL != IM)
    {
      IM->Class = IDCMP_REFRESHWINDOW;
      send_intuimessage(IM, w, IntuitionBase);
    }
  }
}

inline VOID send_intuimessage(struct IntuiMessage *imsg, struct Window *w, struct IntuitionBase *IntuitionBase)
{

    /* Mark the message as taken */    

    /* Reply the message to intuition */
    imsg->ExecMessage.mn_ReplyPort = w->WindowPort;
    imsg->IDCMPWindow = w;
    
    PutMsg(w->UserPort, (struct Message *)imsg);
}

inline VOID free_intuimessage(struct IntuiMessage *imsg,  struct IntuitionBase *IntuitionBase)
{
    FreeMem(imsg, sizeof (struct ExtIntuiMessage));
}

inline struct IntuiMessage *alloc_intuimessage(struct IntuitionBase *IntuitionBase)
{
    struct IntuiMessage	*imsg;
    
    imsg = AllocMem(sizeof(struct ExtIntuiMessage), MEMF_CLEAR|MEMF_PUBLIC);

    return imsg;
}
