/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Support functions for Intuition's InputHandler
    Lang: english
*/

/****************************************************************************************/

#define AROS_ALMOST_COMPATIBLE 1 /* NEWLIST macro */
#include <proto/exec.h>
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
#include "maybe_boopsi.h"
#include "inputhandler_support.h"
#include "menus.h"

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

#include <stddef.h>

/****************************************************************************************/

/*
  All screens and windows will be updated with the current position of
  the mouse pointer. The windows will receive relative mouse coordinates.
*/

/****************************************************************************************/

void notify_mousemove_screensandwindows(WORD x, 
                                        WORD y, 
                                        struct IntuitionBase * IntuitionBase)
{
    struct Screen * scr = IntuitionBase->FirstScreen;

    while (NULL != scr)
    {
	struct Window * win = scr->FirstWindow;

	scr->MouseX = x;
	scr->MouseY = y;

	/* 
	** Visit all windows of this screen
	*/

	while (NULL != win)
	{
	    UpdateMouseCoords(win);

	    win = win -> NextWindow;
	}

	scr = scr->NextScreen;
    }
}

/****************************************************************************************/

/*
** All interested windows of all screens will be notified about new 
** preferences
*/

/****************************************************************************************/

void notify_newprefs(struct IntuitionBase * IntuitionBase)
{
    struct Screen * scr = IntuitionBase->FirstScreen;

    while (NULL != scr)
    {
	/* 
	** Visit all windows of this screen
	*/
	struct Window * win = scr->FirstWindow;

	while (NULL != win)
	{
	    /*
	    ** Is this window interested?
	    */
	    if (0 != (win->IDCMPFlags & IDCMP_NEWPREFS))
	    {
        	ih_fire_intuimessage(win,
                        	     IDCMP_NEWPREFS,
                        	     0,
                        	     win,
                        	     IntuitionBase);
	    }
	    win = win->NextWindow;
	}
	scr = scr->NextScreen;
    }
}

/****************************************************************************************/

void send_intuimessage(struct IntuiMessage *imsg, struct Window *w,
		       struct IntuitionBase *IntuitionBase)
{
    SendIntuiMessage(w, imsg);
}

/****************************************************************************************/

void free_intuimessage(struct IntuiMessage *imsg,
		       struct IntuitionBase *IntuitionBase)
{
    FreeIntuiMessage(imsg);
}

/****************************************************************************************/

struct IntuiMessage *alloc_intuimessage(struct Window *w,
    	    	    	    	    	struct IntuitionBase *IntuitionBase)
{
    struct IntuiMessage	*imsg;
    
    imsg = AllocIntuiMessage(w);
    if (imsg)
    {
        if (w)
	{
            if (w->IDCMPFlags & IDCMP_DELTAMOVE)
	    {
		struct IIHData *iihd = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;

	    	imsg->MouseX = iihd->DeltaMouseX;
		imsg->MouseY = iihd->DeltaMouseY;
	    } else {
		imsg->MouseX = w->MouseX;
		imsg->MouseY = w->MouseY;
	    }
	}
    	CurrentTime(&imsg->Seconds, &imsg->Micros);
    }
    
    return imsg;
}

/****************************************************************************************/

BOOL fire_intuimessage(struct Window *w,
                       ULONG Class,
		       UWORD Code,
		       APTR IAddress,
		       struct IntuitionBase *IntuitionBase)
{
    struct IntuiMessage *imsg;
    BOOL    	    	result = FALSE;
    
    if ((w->IDCMPFlags & Class) && (w->UserPort))
    {
	if ((imsg = alloc_intuimessage(w, IntuitionBase)))
	{
	    struct IIHData *iihd = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;

    	    imsg->Class = Class;
	    imsg->Code = Code;
	    imsg->Qualifier = iihd->ActQualifier;
	    
	    if (Class == IDCMP_RAWKEY)
	    {
		/*
		** In case of IDCMP_RAWKEY IntuiMessage->IAddress is a pointer to the
		** prev Code/Qual data, not the data itself (which is the case for
		** IDCMP_VANILLAKEY)
		*/
		
	    	INT_INTUIMESSAGE(imsg)->prevCodeQuals = IAddress;
		imsg->IAddress = &(INT_INTUIMESSAGE(imsg)->prevCodeQuals);
	    }
	    else
	    {
	    	imsg->IAddress = IAddress;
    	    }
	    send_intuimessage(imsg, w, IntuitionBase);

	    result = TRUE;
	}
    }
    
    return result;
}

/****************************************************************************************/

/*
   use ih_fire_intuimessage if A) the inputevent because of which
   you call this function might have to be eaten or modified
   by Intuition or B) an inputevent might have to be created
   by Intuition because of a deferred action.
   
   In any case this function may be called only from inside Intuition's
   InputHandler!!!!!!
*/
     
/****************************************************************************************/

BOOL ih_fire_intuimessage(struct Window * w,
			  ULONG Class,
			  UWORD Code,
			  APTR IAddress,
			  struct IntuitionBase *IntuitionBase)
{
    struct IIHData    *iihd = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;
    struct InputEvent *ie = iihd->ActInputEvent;
    
    BOOL result = fire_intuimessage(w, Class, Code, IAddress, IntuitionBase);
    
    if (result && ie)
    {
        /* was sent as IDCMP to window so eat inputevent */

        ie->ie_Class = IECLASS_NULL;

    }
    else if (ie && (ie->ie_Class != IECLASS_NULL) && !iihd->ActInputEventUsed)
    {

	/* ih_fire_intuimessage was called from inside Intuition's event handling loop */
	
	iihd->ActInputEventUsed = TRUE;
	
        ie->ie_SubClass = 0;
        ie->ie_Code = Code;
	ie->ie_EventAddress = IAddress;
	
        switch(Class)
	{
	    case IDCMP_GADGETUP:
	        /* Note: on the Amiga if a boopsi Gadget which is GA_Immediate
		   and GA_RelVerify immediately in GM_GOACTIVE returns GMR_VERIFY,
		   then this sends IDCMP_GADGETDOWN + IDCMP_GADGETUP. AROS does
		   the same. But for changed inputevents (if window does not have this
		   IDCMP Flags set) there will be only one IECLASS_GADGETDOWN
		*/
		
	        ie->ie_Class = IECLASS_GADGETUP;
		break;
		
	    case IDCMP_GADGETDOWN:
	        ie->ie_Class = IECLASS_GADGETDOWN;
		break;
		
	    case IDCMP_ACTIVEWINDOW:
	    	ie->ie_Class = IECLASS_ACTIVEWINDOW;
		break;
		
	    case IDCMP_INACTIVEWINDOW:
	        ie->ie_Class = IECLASS_INACTIVEWINDOW;
		break;
	
	    case IDCMP_CLOSEWINDOW:
	    	ie->ie_Class = IECLASS_CLOSEWINDOW;
		break;
				
	    case IDCMP_MENUHELP:
	        ie->ie_Class = IECLASS_MENUHELP;
		break;
		
	    case IDCMP_MENUPICK:
	        ie->ie_Class = IECLASS_MENULIST;
		break;
	
	    case IDCMP_MOUSEBUTTONS:
	    case IDCMP_MOUSEMOVE:
	    case IDCMP_RAWKEY:
	    case IDCMP_VANILLAKEY:
	        break;
		
	    default:
	        D(bug("ih_fireintuimessage: unexpected IDCMP (%x) for an inputevent-handling-fireintuimessage!\n", Class));
	        break;
		
	}
    }
    else if (!ie)
    {
         /* ih_fire_intuimessage was called from inside Intuition's defered action handling routines */

	if ((ie = AllocInputEvent(iihd)))
	{
            switch(Class)
	    {
		case IDCMP_NEWSIZE:
	            ie->ie_Class = IECLASS_SIZEWINDOW;
		    break;

		case IDCMP_CHANGEWINDOW:
	            ie->ie_Class = IECLASS_CHANGEWINDOW;
		    break;

		case IDCMP_ACTIVEWINDOW:
	            ie->ie_Class = IECLASS_ACTIVEWINDOW;
		    break;

		case IDCMP_INACTIVEWINDOW:
		    ie->ie_Class = IECLASS_INACTIVEWINDOW;
		    break;
		    
		case IDCMP_GADGETUP:
		    ie->ie_Class = IECLASS_GADGETUP;
		    break;
		    
		case IDCMP_GADGETDOWN:
		    ie->ie_Class = IECLASS_GADGETDOWN;
		    break;
		    
		case IDCMP_REFRESHWINDOW:
		    ie->ie_Class = IECLASS_REFRESHWINDOW;
		    break;
		    
		case IDCMP_NEWPREFS:
		    ie->ie_Class = IECLASS_NEWPREFS;
		    break;

		default:
	            D(bug("ih_fireintuimessage: unexpected IDCMP (0x%X) for a deferred-action-fireintuimessage!\n", Class));
		    break;
		    
	    } /* switch(Class) */
	    
	    ie->ie_Code 	= Code;
	    ie->ie_Qualifier 	= iihd->ActQualifier;
	    ie->ie_EventAddress = IAddress;
	    CurrentTime(&ie->ie_TimeStamp.tv_secs, &ie->ie_TimeStamp.tv_micro);
	    
	    D(bug("ih_fireintuimessage: generated InputEvent. Class = 0x%X  Code = %d  EventAddress = 0x%X\n",
	    	  ie->ie_Class,
		  ie->ie_Code,
		  ie->ie_EventAddress));
	    
	} /* if ((ie = AllocInputEvent(iihd))) */
    }
    
    return result;
}
			  		       
/*********************************************************************/

IPTR Locked_DoMethodA (Object * obj, Msg message, struct IntuitionBase *IntuitionBase)
{
    IPTR rc;

    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock);
    rc = DoMethodA(obj, message);
    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock);
    
    return rc;
}

/****************************************************************************************/

void NotifyDepthArrangement(struct Window *w, struct IntuitionBase *IntuitionBase)
{
    if(w->MoreFlags & WMFLG_NOTIFYDEPTH)
    {
        ih_fire_intuimessage(w,
	                     IDCMP_CHANGEWINDOW,
			     CWCODE_DEPTH,
			     0,
			     IntuitionBase);
    }    
}


/****************************************************************************************/

void PrepareGadgetInfo(struct GadgetInfo *gi, struct Screen *scr, struct Window *win)
{
    gi->gi_Screen	  = scr;
    gi->gi_Window	  = win;
    gi->gi_RastPort   	  = 0;
    gi->gi_Pens.DetailPen = scr->DetailPen;
    gi->gi_Pens.BlockPen  = scr->BlockPen;
    gi->gi_DrInfo	  = &(((struct IntScreen *)gi->gi_Screen)->DInfo);
}

/****************************************************************************************/

void SetGadgetInfoGadget(struct GadgetInfo *gi, struct Gadget *gad)
{
    SET_GI_RPORT(gi, gi->gi_Window, gad);
    gi->gi_Layer = gi->gi_RastPort->Layer;
    GetGadgetDomain(gad, gi->gi_Screen, gi->gi_Window, NULL, &gi->gi_Domain);
}

/****************************************************************************************/

void SetGPIMouseCoords(struct gpInput *gpi, struct Gadget *gad)
{
    struct GadgetInfo *gi = gpi->gpi_GInfo;
    
    WORD mousex, mousey;
    
    if (IS_SCREEN_GADGET(gad) || !gi->gi_Window)
    {
        mousex = gi->gi_Screen->MouseX;
	mousey = gi->gi_Screen->MouseY;    
    }
    else
    {
        mousex = gi->gi_Window->MouseX;
	mousey = gi->gi_Window->MouseY;
    }
    
    gpi->gpi_Mouse.X = mousex - gi->gi_Domain.Left - GetGadgetLeft(gad, gi->gi_Screen, gi->gi_Window, NULL);
    gpi->gpi_Mouse.Y = mousey - gi->gi_Domain.Top  - GetGadgetTop(gad, gi->gi_Screen, gi->gi_Window, NULL);
}

/****************************************************************************************/

void HandleSysGadgetVerify(struct GadgetInfo *gi, struct Gadget *gadget,
    	    	    	   struct IntuitionBase *IntuitionBase)
{
    switch(gadget->GadgetType & GTYP_SYSTYPEMASK)
    {
    	case GTYP_CLOSE:
	    ih_fire_intuimessage(gi->gi_Window,
	    		      	 IDCMP_CLOSEWINDOW,
			      	 0,
			      	 gi->gi_Window,
			      	 IntuitionBase);
	    break;

	case GTYP_WDEPTH:
	    if (FALSE == IsLayerHiddenBySibling(gi->gi_Window->WLayer,FALSE))
	    {
		/* Send window to back */
		WindowToBack(gi->gi_Window);
	    }
	    else
	    {
		/* Send window to front */
		WindowToFront(gi->gi_Window);
	    }		    
	    break;

	case GTYP_WZOOM:
	    ZipWindow(gi->gi_Window);
	    break;

	case GTYP_SDEPTH:
	    if (gi->gi_Screen == IntuitionBase->FirstScreen)
	    {
		ScreenToBack(gi->gi_Screen);
	    }
	    else
	    {
		ScreenToFront(gi->gi_Screen);
	    }
	    break;	    
	    
    } /* switch(gad->GadgetType & GTYP_SYSTYPEMASK) */
}

/****************************************************************************************/

struct Gadget *HandleCustomGadgetRetVal(IPTR retval, struct GadgetInfo *gi, struct Gadget *gadget,
					ULONG termination, BOOL *reuse_event,
					struct IntuitionBase *IntuitionBase)
{					       
    struct IIHData  *iihdata = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;

    if (retval != GMR_MEACTIVE)
    {
	struct gpGoInactive gpgi;

	if (retval & GMR_REUSE)
	    *reuse_event = TRUE;

	if (retval & GMR_VERIFY)
	{
	    if (IS_SYS_GADGET(gadget))
	    {
	    	HandleSysGadgetVerify(gi, gadget, IntuitionBase);
	    }
	    else
	    {
		/* Not a system gadget. Send IDCMP_GADGETUP, but not
		   if it is a screen gadget where gi->gi_Window would
		   be NULL */

	       if ((gadget->Activation & GACT_RELVERIFY) &&
		   (gi->gi_Window))
	       {
		   ih_fire_intuimessage(gi->gi_Window,
	    		      		IDCMP_GADGETUP,
			      		termination & 0x0000FFFF,
			      		gadget,
			      		IntuitionBase);
	       }

	    } /* switch(gad->GadgetType & GTYP_SYSTYPEMASK) */
	    	    
	} /* if (retval & GMR_VERIFY) */

	gpgi.MethodID = GM_GOINACTIVE;
	gpgi.gpgi_GInfo = gi;
	gpgi.gpgi_Abort = 0;

	Locked_DoMethodA((Object *)gadget, (Msg)&gpgi, IntuitionBase);
    	
	if (SYSGADGET_ACTIVE)
	{
	    /* Switch back from Master Drag or Size Gadget to
	       real/original/app Size or Drag Gadget */
	       
	    gadget = iihdata->ActiveSysGadget;
	    iihdata->ActiveSysGadget = NULL;
	    
	    if (IS_BOOPSI_GADGET(gadget))
	    {
	    	Locked_DoMethodA((Object *)gadget, (Msg)&gpgi, IntuitionBase);
	    }
	    
	    retval = 0;
	}
	
	gadget->Activation &= ~GACT_ACTIVEGADGET;
	
	if ((gadget->Flags & GFLG_TABCYCLE) && (retval & GMR_NEXTACTIVE))
	{
	    gadget = FindCycleGadget(gi->gi_Window, gadget, GMR_NEXTACTIVE);
	}
	else if ((gadget->Flags & GFLG_TABCYCLE) && (retval & GMR_PREVACTIVE))
	{
	    gadget = FindCycleGadget(gi->gi_Window, gadget, GMR_PREVACTIVE);
	}
	else
	{
	    gadget = NULL;
	}
	
	if (gadget)
	{
	    gadget = DoActivateGadget(gi->gi_Window, gadget, IntuitionBase);
	}
	
    } /* if (retval != GMR_MEACTIVE) */
    else
    {
	gadget->Activation |= GACT_ACTIVEGADGET;
    }
    
    return gadget;
}

/****************************************************************************************/

struct Gadget *DoGPInput(struct GadgetInfo *gi, struct Gadget *gadget,
			 struct InputEvent *ie, STACKULONG methodid,
			 BOOL *reuse_event, struct IntuitionBase *IntuitionBase)
{
    struct IIHData  *iihdata = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;
    struct gpInput  gpi;
    IPTR    	    retval;
    ULONG   	    termination;
    
    ie->ie_Qualifier = iihdata->ActQualifier;
    
    gpi.MethodID    	= methodid;
    gpi.gpi_GInfo   	= gi;
    gpi.gpi_IEvent  	= ie;
    gpi.gpi_Termination = &termination;
    gpi.gpi_TabletData  = NULL;
    
    SetGPIMouseCoords(&gpi, gadget);
    
    retval = Locked_DoMethodA ((Object *)gadget, (Msg)&gpi, IntuitionBase);

    return HandleCustomGadgetRetVal(retval, gi, gadget, termination,
				    reuse_event, IntuitionBase);
    
}

/****************************************************************************************/

struct Gadget * FindGadget (struct Screen *scr, struct Window * window, int x, int y,
			    struct GadgetInfo * gi, struct IntuitionBase *IntuitionBase)
{
    struct Gadget   	*gadget, *firstgadget;
    struct gpHitTest 	gpht;
    struct IBox     	ibox;
    WORD    	    	i, xrel, yrel;

    gpht.MethodID     = GM_HITTEST;
    gpht.gpht_GInfo   = gi;

    if (window)
    {
        firstgadget = window->FirstGadget;
    }
    else
    {
        firstgadget = scr->FirstGadget;
    }
    
    for(i = 0; i < 2; i++)
    {    
	for (gadget = firstgadget; gadget; gadget = gadget->NextGadget)
	{
    	    if (!(gadget->Flags & GFLG_DISABLED))
	    {
    		/* stegerg: domain depends on gadgettype and windowflags! */
        	GetGadgetDomain(gadget, scr, window, NULL, &gi->gi_Domain);

		/* Get coords relative to window */

    		GetGadgetIBox((Object *)gadget, gi, &ibox);

		xrel = x - gi->gi_Domain.Left;
		yrel = y - gi->gi_Domain.Top;
		
    	    	if ((i == 0) && window)
		{
		    xrel -= window->LeftEdge;
		    yrel -= window->TopEdge;
		}
		
		if ((xrel >= ibox.Left) &&
		    (yrel >= ibox.Top) &&
		    (xrel < ibox.Left + ibox.Width) &&  
		    (yrel < ibox.Top  + ibox.Height))
		{
	    	    if ((gadget->GadgetType & GTYP_GTYPEMASK) != GTYP_CUSTOMGADGET) break;

		    gpht.gpht_Mouse.X = xrel - ibox.Left;
		    gpht.gpht_Mouse.Y = yrel - ibox.Top;

		    if (Locked_DoMethodA ((Object *)gadget, (Msg)&gpht, IntuitionBase) == GMR_GADGETHIT) break;

		}

	    } /* if (!(gadget->Flags & GFLG_DISABLED)) */

	} /* for (gadget = window->FirstGadget; gadget; gadget = gadget->NextGadget) */

    	if (gadget || !window) break;
	
	firstgadget = scr->FirstGadget;
    }
    
    return (gadget);

} /* FindGadget */


/****************************************************************************************/

BOOL InsideGadget(struct Screen *scr, struct Window *win,
    	    	  struct Gadget *gad, WORD x, WORD y)
{
    struct IBox box;
    BOOL    	rc = FALSE;
    
    GetScrGadgetIBox(gad, scr, win, NULL, &box);
       
    if ((x >= box.Left) &&
    	(y >= box.Top)  &&
	(x < box.Left + box.Width) &&
	(y < box.Top + box.Height))
    {
    	rc = TRUE;
    }
    
    return rc; 
}

/****************************************************************************************/

struct Gadget *DoActivateGadget(struct Window *win, struct Gadget *gad,
    	    	    	    	struct IntuitionBase *IntuitionBase)
{
    struct IIHData  	*iihd = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;
    struct GadgetInfo 	*gi = &iihd->GadgetInfo;
    struct Gadget   	*result = NULL;

    if (gad->Activation & GACT_IMMEDIATE)
    {
	ih_fire_intuimessage(win,
			     IDCMP_GADGETDOWN,
			     0,
			     gad,
			     IntuitionBase);
    }

    PrepareGadgetInfo(gi, win->WScreen, win);
    SetGadgetInfoGadget(gi, gad);
    
    switch(gad->GadgetType & GTYP_GTYPEMASK)
    {
        case GTYP_STRGADGET:
	    gad->Activation |= GACT_ACTIVEGADGET;
	    UpdateStrGadget(gad, win, IntuitionBase);
	    result = gad;
	    break;
	    
	case GTYP_CUSTOMGADGET:
	{
	    struct gpInput  gpi;
	    ULONG   	    termination;
	    IPTR    	    retval;
	    BOOL    	    reuse_event;

	    gpi.MethodID    	= GM_GOACTIVE;
	    gpi.gpi_GInfo	= gi;
	    gpi.gpi_IEvent	= NULL;
	    gpi.gpi_Termination = &termination;
	    gpi.gpi_Mouse.X 	= win->MouseX - gi->gi_Domain.Left - GetGadgetLeft(gad, gi->gi_Screen, gi->gi_Window, NULL);
	    gpi.gpi_Mouse.Y 	= win->MouseY - gi->gi_Domain.Top  - GetGadgetTop(gad, gi->gi_Screen, gi->gi_Window, NULL);
	    gpi.gpi_TabletData	= NULL;

	    retval = Locked_DoMethodA ((Object *)gad, (Msg)&gpi, IntuitionBase);
	    
	    gad = HandleCustomGadgetRetVal(retval, gi, gad,termination,
					   &reuse_event, IntuitionBase);

	    if (gad)
	    {
		gad->Activation |= GACT_ACTIVEGADGET;
	        result = gad;
	    }
	    break;
	}
	    
    } /* switch(gad->GadgetType & GTYP_GTYPEMASK) */
    
    if (result) iihd->ActiveGadget = result;
 
    return result;
}


/****************************************************************************************/

struct Gadget *FindCycleGadget(struct Window *win, struct Gadget *gad, WORD direction)
{
    struct Gadget *g, *gg, *prev;
    
    D(bug("FindCycleGadget: win = %x  gad = %x  direction = %d\n", win, gad, direction));
    
    switch(direction)
    {
        case GMR_NEXTACTIVE:
	    g = gad->NextGadget;
	    if (!g) g = win->FirstGadget;
	    
	    while(g)
	    {
	        if (g == gad)
		{
		    if (!(gad->Flags & GFLG_TABCYCLE) || (gad->Flags & GFLG_DISABLED))
		    {
		        /* should never happen */
			g = NULL;
		    }
		    break;
		}
		if (!(g->Flags & GFLG_DISABLED) && (g->Flags & GFLG_TABCYCLE)) break;
		
		g = g->NextGadget;
		if (!g) g = win->FirstGadget;
	    }
	    break;
	    
	case GMR_PREVACTIVE:
	    prev = 0;g = 0;
	    gg = win->FirstGadget;
	    
	    /* find a TABCYCLE gadget which is before gad in window's gadgetlist */
	    while (gg)
	    {
	        if (gg == gad)
		{
		    if (prev) g = prev;
		    break;
		}
		if (!(gg->Flags & GFLG_DISABLED) && (gg->Flags & GFLG_TABCYCLE)) prev = gg;
		gg = gg->NextGadget;
	    }
	    
	    if (gg && !g)
	    {
	        /* There was no TABCYCLE gadget before gad in window's gadgetlist */
		
	        gg = gg->NextGadget;
		if (!gg)
		{
		    if (!(gad->Flags & GFLG_DISABLED) && (gad->Flags & GFLG_TABCYCLE)) g = gad;
		    break;
		}
		prev = 0;
		
		while(gg)
		{
		    if (!(gg->Flags & GFLG_DISABLED) && (gg->Flags & GFLG_TABCYCLE)) prev = gg;
		    gg = gg->NextGadget;
		}
		
		if (prev)
		{
		    g = prev;
		}
		else
		{
		    if (!(gad->Flags & GFLG_DISABLED) && (gad->Flags & GFLG_TABCYCLE)) g = gad;
		}
	    }    
		    
		
	    break;

    } /* switch(direction) */
    
    return g;
}

/****************************************************************************************/

void FixWindowCoords(struct Window *win, WORD *left, WORD *top, WORD *width, WORD *height)
{
    struct Screen *scr = win->WScreen;
    
    if (*width < 1) *width = 1;
    if (*height < 1) *height = 1;

    if (!(win->WScreen->LayerInfo.Flags & LIFLG_SUPPORTS_OFFSCREEN_LAYERS))
    {
	if (*width > scr->Width) *width = scr->Width;
	if (*height > scr->Height) *height = scr->Height;

	if ((*left + *width) > scr->Width)
	{
    	    *left = scr->Width - *width;
	}
	else if (*left < 0)
	 {
    	    *left = 0;
	}

	if ((*top + *height) > scr->Height)
	{
    	    *top = scr->Height - *height;
	}
	else if (*top < 0)
	{
    	    *top = 0;
	}
    }
}

/****************************************************************************************/

void WindowNeedsRefresh(struct Window * w, 
                        struct IntuitionBase * IntuitionBase )
{
    /* Supposed to send a message to this window, saying that it needs a
       refresh. I will check whether there is no such a message queued in
       its messageport, though. It only needs one such message! 
    */

    /* Refresh the window's gadgetry ... 
       ... stegerg: and in the actual implementation
       call RefershWindowFrame first, as the border gadgets don´t
       cover the whole border area.*/

    Gad_BeginUpdate(w->WLayer, IntuitionBase);

    if (!IS_GZZWINDOW(w)) RefreshWindowFrame(w);

    /* refresh all gadgets except border gadgets, because they
       were already refreshed in refreshwindowframe */
    int_refreshglist(w->FirstGadget, w, NULL, -1, 0, REFRESHGAD_BORDER, IntuitionBase);
    if (IS_NOCAREREFRESH(w)) w->WLayer->Flags &= ~LAYERREFRESH;

    Gad_EndUpdate(w->WLayer, IS_NOCAREREFRESH(w), IntuitionBase);

    if (IS_DOCAREREFRESH(w))
    {
        if (w->UserPort && (w->IDCMPFlags & IDCMP_REFRESHWINDOW))
	{
            struct IntuiMessage *IM;
            BOOL found = FALSE;

	    /* Can use Forbid() for this */
	    Forbid();

	    IM = (struct IntuiMessage *)w->UserPort->mp_MsgList.lh_Head;

	    ForeachNode(&w->UserPort->mp_MsgList, IM)
	    {
		/* Does the window already have such a message? */
		if (IDCMP_REFRESHWINDOW == IM->Class)
		{
        	    D(bug("Window %s already has a refresh message pending!!\n",
		    	   w->Title ? w->Title : (STRPTR)"<NONAME>"));
		    found = TRUE;break;
		}
	    }

	    Permit();
	
	    if (!found)
	    {
        	D(bug("Sending a refresh message to window %s  %d %d %d %d!!\n",
		      w->Title ? w->Title : (STRPTR)"<NONAME>",
		      w->LeftEdge,
		      w->TopEdge,
		      w->Width,
		      w->Height));

		fire_intuimessage(w,
                		  IDCMP_REFRESHWINDOW,
				  0, 
				  w, 
				  IntuitionBase);
	    } /* if (!found) */
	    
	} /* if (w->UserPort && (w->IDCMPFlags & IDCMP_REFRESHWINDOW)) */
	else
	{
	    struct IIHData *iihdata = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;
	    
	    if (FindTask(NULL) == iihdata->InputDeviceTask)
	    {
	        ih_fire_intuimessage(w,
				     IDCMP_REFRESHWINDOW,
				     0,
				     w,
				     IntuitionBase);
	    }
	}
	
    } /* if (!IS_NOCAREREFRESH(w)) */
    
}

/****************************************************************************************/

struct Window *FindActiveWindow(struct InputEvent *ie, BOOL *swallow_event,
				struct IntuitionBase *IntuitionBase)
{
    /* The caller has checked that the input event is a IECLASS_RAWMOUSE, SELECTDOWN event */
    struct Screen   *scr;
    struct Layer    *l;
    struct Window   *new_w;
    ULONG   	    lock;
    
    *swallow_event = FALSE;

    lock = LockIBase(0UL);
    
    new_w = IntuitionBase->ActiveWindow;
    scr   = IntuitionBase->ActiveScreen;
    
    UnlockIBase(lock);
    
    if (scr)
    {
	/* What layer ? */
	D(bug("FindActiveWindow: Click at (%d,%d)\n",scr->MouseX,scr->MouseY));
	LockLayerInfo(&scr->LayerInfo);

	l = WhichLayer(&scr->LayerInfo, scr->MouseX, scr->MouseY);

	UnlockLayerInfo(&scr->LayerInfo);

	if (NULL == l)
	{
    	    new_w = NULL;
	    D(bug("FindActiveWindow: Click not inside layer\n"));
	}
	else if (l == scr->BarLayer)
	{
    	    D(bug("FindActiveWindow: Click on screen bar layer -> active window stays the same\n"));
	}
	else
	{
	    new_w = (struct Window *)l->Window;
	    if (!new_w)
	    {
		D(bug("FindActiveWindow: Selected layer is not a window\n"));
	    }

    	    D(bug("FindActiveWindow: Found layer %p\n", l));

	}
    }
    
    return new_w;
}

/****************************************************************************************/

struct InputEvent *AllocInputEvent(struct IIHData *iihdata)
{
    struct IntuitionBase    	*IntuitionBase = iihdata->IntuitionBase;
    struct GeneratedInputEvent  *ie;
    
    ie = AllocPooled(iihdata->InputEventMemPool, sizeof(struct GeneratedInputEvent));
    if (ie)
    {
        if (iihdata->ActGeneratedInputEvent)
	{
	    iihdata->ActGeneratedInputEvent->ie_NextEvent = &ie->ie;
	    iihdata->ActGeneratedInputEvent = &ie->ie;
	} else {
	    iihdata->GeneratedInputEvents = &ie->ie;
	    iihdata->ActGeneratedInputEvent = &ie->ie;
	}
	AddTail((struct List *)&iihdata->GeneratedInputEventList, (struct Node *)&ie->node);
    }
    
    return (struct InputEvent *)ie;
}

/****************************************************************************************/

void FreeGeneratedInputEvents(struct IIHData *iihdata)
{
    struct IntuitionBase    	*IntuitionBase = iihdata->IntuitionBase;
    struct Node     	    	*node, *succ;
    struct GeneratedInputEvent  *ie;
    
    ForeachNodeSafe(&iihdata->GeneratedInputEventList, node, succ)
    {
	ie = (struct GeneratedInputEvent *)(((UBYTE *)node) - offsetof(struct GeneratedInputEvent, node));
	FreePooled(iihdata->InputEventMemPool, ie, sizeof(struct GeneratedInputEvent));
    }
    
    iihdata->GeneratedInputEvents = NULL;
    iihdata->ActGeneratedInputEvent = NULL;
    
    NEWLIST(&iihdata->GeneratedInputEventList);
}

/****************************************************************************************/

BOOL FireMenuMessage(WORD code, struct Window *win,
		     struct InputEvent *ie, struct IntuitionBase *IntuitionBase)
{
    struct MenuMessage *msg;
    BOOL    	       result = FALSE;
    
    if ((msg = AllocMenuMessage(IntuitionBase)))
    {
    	msg->code = code;
	msg->win  = win;
	msg->ie   = *ie;
	SendMenuMessage(msg, IntuitionBase);
	
	result = TRUE;
    }
    
    return result;
}
    
/****************************************************************************************/

LONG Gad_BeginUpdate(struct Layer *layer, struct IntuitionBase *IntuitionBase)
{
    /* Must lock GadgetLock to avoid deadlocks with ObtainGirPort
       from other tasks, because ObtainGirPort first obtains
       GadgetLock and then layer lock through LockLayer!!!! */
       
    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock);

    return BeginUpdate(layer);
}

/****************************************************************************************/

void Gad_EndUpdate(struct Layer *layer, UWORD flag, struct IntuitionBase *IntuitionBase)
{
    EndUpdate(layer, flag);

    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock);    
}

/****************************************************************************************/
