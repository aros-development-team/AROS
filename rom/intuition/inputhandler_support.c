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

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

/*
  All screens and windows will be updated with the current position of
  the mouse pointer. The windows will receive relative mouse coordinates.
*/
void notify_mousemove_screensandwindows(WORD x, 
                                        WORD y, 
                                        struct IntuitionBase * IntuitionBase)
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
      win->MouseX = x - win->LeftEdge;
      win->MouseY = y - win->TopEdge;
      
      /* stegerg: AmigaOS sets this even if window is not GZZ
         so we do the same as they are handy also for non-GZZ
	 windows */
	 
      win->GZZMouseX = x - (win->LeftEdge + win->BorderLeft);
      win->GZZMouseY = y - (win->TopEdge  + win->BorderTop);

      win = win -> NextWindow;
    }
    scr->MouseX = x;
    scr->MouseY = y;
    scr = scr->NextScreen;
  }
}

/*********************************************************************
** All interested windows of all screens will be notified about new 
** preferences
*/
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

/*********************************************************************/

void send_intuimessage(struct IntuiMessage *imsg,
		       struct Window *w,
		       struct IntuitionBase *IntuitionBase)
{
    /* Mark the message as taken */    

    /* Reply the message to intuition */
    imsg->ExecMessage.mn_ReplyPort = w->WindowPort;

    imsg->IDCMPWindow = w;
    
    PutMsg(w->UserPort, (struct Message *)imsg);
}

/*********************************************************************/

void free_intuimessage(struct IntuiMessage *imsg,
		       struct IntuitionBase *IntuitionBase)
{
    FreeMem(imsg, sizeof (struct ExtIntuiMessage));
}

/*********************************************************************/

struct IntuiMessage *alloc_intuimessage(struct Window *w,
					struct IntuitionBase *IntuitionBase)
{
    struct IntuiMessage	*imsg;
    
    imsg = AllocMem(sizeof(struct ExtIntuiMessage), MEMF_CLEAR|MEMF_PUBLIC);
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
	    
	    imsg->IDCMPWindow = w;
	}
    	CurrentTime(&imsg->Seconds, &imsg->Micros);
    }
    
    return imsg;
}

/*********************************************************************/

BOOL fire_intuimessage(struct Window *w,
                       ULONG Class,
		       UWORD Code,
		       APTR IAddress,
		       struct IntuitionBase *IntuitionBase)
{
    struct IntuiMessage *imsg;
    BOOL result = FALSE;
    
    if ((w->IDCMPFlags & Class) && (w->UserPort))
    {
	if ((imsg = alloc_intuimessage(w, IntuitionBase)))
	{
	    struct IIHData *iihd = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;

    	    imsg->Class = Class;
	    imsg->Code = Code;
	    imsg->Qualifier = iihd->ActQualifier;
	    imsg->IAddress = IAddress;

	    send_intuimessage(imsg, w, IntuitionBase);

	    result = TRUE;
	}
    }
    
    return result;
}

/*********************************************************************/

/* use ih_fire_intuimessage if A) the inputevent because of which
   you call this function might have to be eaten or modified
   by Intuition or B) an inputevent might have to be created
   by Intuition because of a deferred action.
   
   In any case this function may be called only from inside Intuition's
   InputHandler!!!!!! */
     
BOOL ih_fire_intuimessage(struct Window * w,
			  ULONG Class,
			  UWORD Code,
			  APTR IAddress,
			  struct IntuitionBase *IntuitionBase)
{
    struct IIHData *iihd = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;
    struct InputEvent *ie = iihd->ActInputEvent;
    
    BOOL result = fire_intuimessage(w, Class, Code, IAddress, IntuitionBase);
    
    if (result && ie)
    {
        /* was sent as IDCMP to window so eat inputevent */

        ie->ie_Class = IECLASS_NULL;

    } else if (ie && (ie->ie_Class != IECLASS_NULL) && !iihd->ActInputEventUsed) {

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
    } else if (!ie)
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
		    
		case IDCMP_CLOSEWINDOW:
		    ie->ie_Class = IECLASS_CLOSEWINDOW;
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

/*************************
**  Locked_DoMethodA	**
*************************/
IPTR Locked_DoMethodA (Object * obj, Msg message, struct IntuitionBase *IntuitionBase)
{
    IPTR rc;

    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock);
    rc = DoMethodA(obj, message);
    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock);
    
    return rc;
}

/*********************************************************************/

/******************************
 **  NotifyDepthArrangement  **
 *****************************/

void NotifyDepthArrangement(struct Window *w,
		       	    struct IntuitionBase *IntuitionBase)
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


/*********************************************************************/

/************************
**  PrepareGadgetInfo  **
************************/
void PrepareGadgetInfo(struct GadgetInfo *gi, struct Screen *scr, struct Window *win)
{
    gi->gi_Screen	  = scr;
    gi->gi_Window	  = win;
    gi->gi_RastPort   	  = 0;
    gi->gi_Pens.DetailPen = scr->DetailPen;
    gi->gi_Pens.BlockPen  = scr->BlockPen;
    gi->gi_DrInfo	  = &(((struct IntScreen *)gi->gi_Screen)->DInfo);
}


/**************************
**  SetGadgetInfoGadget  **
**************************/
void SetGadgetInfoGadget(struct GadgetInfo *gi, struct Gadget *gad)
{
    SET_GI_RPORT(gi, gi->gi_Window, gad);
    GetGadgetDomain(gad, gi->gi_Screen, gi->gi_Window, NULL, &gi->gi_Domain);
}
/***********************
**  SetGIMouseCoords  **
***********************/
void SetGPIMouseCoords(struct gpInput *gpi, struct Gadget *gad)
{
    struct GadgetInfo *gi = gpi->gpi_GInfo;
    
    WORD mousex, mousey;
    
    if (gi->gi_Window)
    {
        mousex = gi->gi_Window->MouseX;
	mousey = gi->gi_Window->MouseY;
    } else {
        mousex = gi->gi_Screen->MouseX;
	mousey = gi->gi_Screen->MouseY;
    }
    
    gpi->gpi_Mouse.X = mousex - gi->gi_Domain.Left - GetGadgetLeft(gad, gi->gi_Screen, gi->gi_Window, NULL);
    gpi->gpi_Mouse.Y = mousey - gi->gi_Domain.Top  - GetGadgetTop(gad, gi->gi_Screen, gi->gi_Window, NULL);
}

/*******************************
**  HandleCustomGadgetRetVal  **
*******************************/
struct Gadget *HandleCustomGadgetRetVal(IPTR retval, struct GadgetInfo *gi, struct Gadget *gadget,
					ULONG termination,
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
	    ih_fire_intuimessage(gi->gi_Window,
	    		      	 IDCMP_GADGETUP,
			      	 termination & 0x0000FFFF,
			      	 gadget,
			      	 IntuitionBase);
	}

	gpgi.MethodID = GM_GOINACTIVE;
	gpgi.gpgi_GInfo = gi;
	gpgi.gpgi_Abort = 0;

	Locked_DoMethodA((Object *)gadget, (Msg)&gpgi, IntuitionBase);

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
	} /* if (gadget) */
	
    } /* if (retval != GMR_MEACTIVE) */
    else
    {
	gadget->Activation |= GACT_ACTIVEGADGET;
    }
    
    return gadget;
}

/****************
**  DoGPInput  **
****************/
struct Gadget *DoGPInput(struct GadgetInfo *gi, struct Gadget *gadget,
			 struct InputEvent *ie, STACKULONG methodid,
			 BOOL *reuse_event, struct IntuitionBase *IntuitionBase)
{
    struct gpInput gpi;
    IPTR retval;
    ULONG termination;
    
    gpi.MethodID = methodid;
    gpi.gpi_GInfo = gi;
    gpi.gpi_IEvent = ie;
    gpi.gpi_Termination = &termination;
    gpi.gpi_TabletData = NULL;
    SetGPIMouseCoords(&gpi, gadget);
    
    retval = Locked_DoMethodA ((Object *)gadget, (Msg)&gpi, IntuitionBase);

    return HandleCustomGadgetRetVal(retval, gi, gadget, termination,
				    reuse_event, IntuitionBase);
    
}

/*********************************************************************/

/*****************
**  FindGadget	**
*****************/
struct Gadget * FindGadget (struct Screen *scr, struct Window * window, int x, int y,
			    struct GadgetInfo * gi, struct IntuitionBase *IntuitionBase)
{
    struct Gadget * gadget, * firstgadget;
    struct gpHitTest gpht;
    struct IBox ibox;
    WORD xrel, yrel;

    gpht.MethodID     = GM_HITTEST;
    gpht.gpht_GInfo   = gi;

    if (window)
    {
        firstgadget = window->FirstGadget;
    } else {
        firstgadget = scr->FirstGadget;
    }
    
    for (gadget = firstgadget; gadget; gadget = gadget->NextGadget)
    {
    	if (!(gadget->Flags & GFLG_DISABLED))
	{
    	    /* stegerg: domain depends on gadgettype and windowflags! */
            GetGadgetDomain(gadget, scr, window, NULL, &gi->gi_Domain);

	    /* Get coords relative to window */

    	    GetGadgetIBox((Object *)gadget, gi, &ibox);

	    xrel = x - gi->gi_Domain.Left - (window ? window->LeftEdge : 0);
	    yrel = y - gi->gi_Domain.Top  - (window ? window->TopEdge : 0);

	    if (   xrel >= ibox.Left
		&& yrel >= ibox.Top
		&& xrel < ibox.Left + ibox.Width 
		&& yrel < ibox.Top  + ibox.Height )
	    {
	    	if ((gadget->GadgetType & GTYP_GTYPEMASK) != GTYP_CUSTOMGADGET) break;
	  
		gpht.gpht_Mouse.X = xrel - ibox.Left;
		gpht.gpht_Mouse.Y = yrel - ibox.Top;

		if (Locked_DoMethodA ((Object *)gadget, (Msg)&gpht, IntuitionBase) == GMR_GADGETHIT) break;

	    }

	} /* if (!(gadget->Flags & GFLG_DISABLED)) */

    } /* for (gadget = window->FirstGadget; gadget; gadget = gadget->NextGadget) */

    return (gadget);

} /* FindGadget */


/*******************
**  InsideGadget  **
*******************/
BOOL InsideGadget(struct Screen *scr, struct Window *win, struct Gadget *gad,
		  WORD x, WORD y)
{
    struct IBox box;
    BOOL rc = FALSE;
    
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


/***********************
**  DoActivateGadget  **
***********************/
struct Gadget *DoActivateGadget(struct Window *win, struct Gadget *gad, struct IntuitionBase *IntuitionBase)
{
    struct IIHData *iihd = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;
    struct GadgetInfo *gi = &iihd->GadgetInfo;
    struct Gadget *result = NULL;

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
	    
	case GTYP_CUSTOMGADGET: {
	    struct gpInput gpi;
	    ULONG termination;
	    IPTR retval;
	    BOOL reuse_event;

	    gpi.MethodID = GM_GOACTIVE;
	    gpi.gpi_GInfo	= gi;
	    gpi.gpi_IEvent	= NULL;
	    gpi.gpi_Termination = &termination;
	    gpi.gpi_Mouse.X = win->MouseX - gi->gi_Domain.Left - GetGadgetLeft(gad, gi->gi_Screen, gi->gi_Window, NULL);
	    gpi.gpi_Mouse.Y = win->MouseY - gi->gi_Domain.Top  - GetGadgetTop(gad, gi->gi_Screen, gi->gi_Window, NULL);
	    gpi.gpi_TabletData	= NULL;

	    retval = Locked_DoMethodA ((Object *)gad, (Msg)&gpi, IntuitionBase);
	    gad = HandleCustomGadgetRetVal(retval, gi, gad,termination,
					   &reuse_event, IntuitionBase);

	    if (gad)
	    {
		gad->Activation |= GACT_ACTIVEGADGET;
	        result = gad;
	    }
	    break; }
	    
    } /* switch(gad->GadgetType & GTYP_GTYPEMASK) */
    
    if (result) iihd->ActiveGadget = result;
 
    return result;
}


/**********************
**  FindCycleGadget  **
**********************/
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
	        if ((g == gad) && (!(gad->Flags & GFLG_TABCYCLE)))
		{
		    /* should never happen */
		    g = NULL;break;
		}
		if (g->Flags & GFLG_TABCYCLE) break;
		
		g = g->NextGadget;
		if (!g) g = win->FirstGadget;
	    }
	    break;
	    
	case GMR_PREVACTIVE:
	    prev = 0;g = 0;
	    gg = win->FirstGadget;
	    while (gg)
	    {
	        if (gg == gad)
		{
		    if (prev) g = prev;
		    break;
		}
		if (gg->Flags & GFLG_TABCYCLE) prev = gg;
		gg = gg->NextGadget;
	    }
	    
	    if (gg && !g)
	    {
	        gg = gg->NextGadget;
		if (!gg)
		{
		    if (gad->Flags & GFLG_TABCYCLE) g = gad;
		    break;
		}
		prev = 0;
		
		while(gg)
		{
		    if (gg->Flags & GFLG_TABCYCLE) prev = gg;
		    gg = gg->NextGadget;
		}
		
		if (prev)
		{
		    g = prev;
		} else {
		    if (gad->Flags & GFLG_TABCYCLE) g = gad;
		}
	    }    
		    
		
	    break;

    } /* switch(direction) */
    
    return g;
}

/*********************************************************************/

/**********************
**  FixWindowCoords  **
**********************/
void FixWindowCoords(struct Window *win, WORD *left, WORD *top, WORD *width, WORD *height)
{
    struct Screen *scr = win->WScreen;
    
    if (*width > scr->Width) *width = scr->Width;
    if (*height > scr->Height) *height = scr->Height;

    if ((*left + *width) > scr->Width)
    {
    	*left = scr->Width - *width;
    } else if (*left < 0) {
    	*left = 0;
    }
    
    if ((*top + *height) > scr->Height)
    {
    	*top = scr->Height - *height;
    } else if (*top < 0) {
    	*top = 0;
    }
}

/*********************************************************************/

/*************************
**  WindowNeedsRefresh	**
*************************/
void WindowNeedsRefresh(struct Window * w, 
                        struct IntuitionBase * IntuitionBase )
{
  /* Supposed to send a message to this window, saying that it needs a
     refresh. I will check whether there is no such a message queued in
     its messageport, though. It only needs one such message! 
  */
  struct IntuiMessage * IM;
  BOOL found = FALSE;

  if (NULL == w->UserPort)
    return;
  
  
  /* Refresh the window's gadgetry ... 
     ... stegerg: and in the actual implementation
     call RefershWindowFrame first, as the border gadgets don´t
     cover the whole border area.*/

  
  if (FALSE != BeginUpdate(w->WLayer))
  {
  
    if (!(w->Flags & WFLG_GIMMEZEROZERO))
    {
      RefreshWindowFrame(w);
    }
  
    /* refresh all gadgets except border gadgets, because they
       were already refreshed in refreshwindowframe */
    int_refreshglist(w->FirstGadget, w, NULL, -1, 0, REFRESHGAD_BORDER, IntuitionBase);
  }

  EndUpdate(w->WLayer, FALSE);
  
  /* Can use Forbid() for this */
  
  Forbid();
  
  IM = (struct IntuiMessage *)w->UserPort->mp_MsgList.lh_Head;

  /* reset the flag in the layer */
  w->WLayer->Flags &= ~LAYERREFRESH;

  ForeachNode(&w->UserPort->mp_MsgList, IM)
  {
    /* Does the window already have such a message? */
    if (IDCMP_REFRESHWINDOW == IM->Class)
    {
kprintf("Window %s already has a refresh message pending!!\n",w->Title);
      found = TRUE;break;
    }
  }
  
  Permit();

kprintf("Sending a refresh message to window %s!!\n",w->Title);
  if (!found)
  {
    IM = alloc_intuimessage(w, IntuitionBase);
    if (NULL != IM)
    {
      IM->Class = IDCMP_REFRESHWINDOW;
      send_intuimessage(IM, w, IntuitionBase);
    }
  }
}

/***********************
**  FindActiveWindow  **
***********************/
struct Window *FindActiveWindow(struct InputEvent *ie, BOOL *swallow_event,
				struct IntuitionBase *IntuitionBase)
{
    /* The caller has checked that the input event is a IECLASS_RAWMOUSE, SELECTDOWN event */
    struct Screen *scr;
    struct Layer *l;
    struct Window *new_w = NULL;
    ULONG lock;
    
    *swallow_event = FALSE;

    lock = LockIBase(0UL);
    scr = IntuitionBase->ActiveScreen;
    UnlockIBase(lock);
    
    /* What layer ? */
    D(bug("Click at (%d,%d)\n",ie->ie_X,ie->ie_Y));
    LockLayerInfo(&scr->LayerInfo);

    l = WhichLayer(&scr->LayerInfo, ie->ie_X, ie->ie_Y);

    UnlockLayerInfo(&scr->LayerInfo);

    if (NULL == l)
    {
	D(bug("iih: Click not inside layer\n"));
    }
    else
    {
	new_w = (struct Window *)l->Window;
	if (!new_w)
	{
	    D(bug("iih: Selected layer is not a window\n"));
	}

    	D(bug("Found layer %p\n", l));

    }

    return new_w;
}

/*********************************************************************/

struct InputEvent *AllocInputEvent(struct IIHData *iihdata)
{
    struct IntuitionBase *IntuitionBase = iihdata->IntuitionBase;
    struct InputEvent *ie;
    
    ie = AllocPooled(iihdata->InputEventMemPool, sizeof(struct InputEvent));
    if (ie)
    {
        if (iihdata->ActGeneratedInputEvent)
	{
	    iihdata->ActGeneratedInputEvent->ie_NextEvent = ie;
	    iihdata->ActGeneratedInputEvent = ie;
	} else {
	    iihdata->GeneratedInputEvents = ie;
	    iihdata->ActGeneratedInputEvent = ie;
	}
    }
    
    return ie;
}

void FreeGeneratedInputEvents(struct IIHData *iihdata)
{
    struct IntuitionBase *IntuitionBase = iihdata->IntuitionBase;
    struct InputEvent *ie, *nextie;
    
    ie = iihdata->GeneratedInputEvents;
    while (ie)
    {
        nextie = ie->ie_NextEvent;
	
	FreePooled(iihdata->InputEventMemPool, ie, sizeof(struct InputEvent));
	
	ie = nextie;    
    }
    
    iihdata->GeneratedInputEvents = NULL;
    iihdata->ActGeneratedInputEvent = NULL;
}

/*********************************************************************/
/*********************************************************************/
/*********************************************************************/
/*********************************************************************/
