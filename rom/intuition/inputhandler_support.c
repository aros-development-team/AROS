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
            imsg->MouseX = w->MouseX;
	    imsg->MouseY = w->MouseY;
	    imsg->IDCMPWindow = w;
	}
    	CurrentTime(&imsg->Seconds, &imsg->Micros);
    }
    
    return imsg;
}

/*********************************************************************/

void fire_intuimessage(struct Window *w,
                       ULONG Class,
		       UWORD Code,
		       APTR IAddress,
		       struct IntuitionBase *IntuitionBase)
{
    struct IntuiMessage *imsg;
    
    if ((w->IDCMPFlags & Class) && (w->UserPort))
    {
	if ((imsg = alloc_intuimessage(w, IntuitionBase)))
	{
	    struct IIHData *iih = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;

    	    imsg->Class = Class;
	    imsg->Code = Code;
	    imsg->Qualifier = iih->ActQualifier;
	    imsg->IAddress = IAddress;

	    send_intuimessage(imsg, w, IntuitionBase);
	}
    }    
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
        fire_intuimessage(w,
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
void PrepareGadgetInfo(struct GadgetInfo *gi, struct Window *win)
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
void SetGadgetInfoGadget(struct GadgetInfo *gi, struct Gadget *gad)
{
    SET_GI_RPORT(gi, gi->gi_Window, gad);
    GetGadgetDomain(gad, gi->gi_Window, NULL, &gi->gi_Domain);
}

/*******************************
**  HandleCustomGadgetRetVal  **
*******************************/
struct Gadget *HandleCustomGadgetRetVal(IPTR retval, struct GadgetInfo *gi, struct Gadget *gadget,
					ULONG *termination,
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
	    fire_intuimessage(gi->gi_Window,
	    		      IDCMP_GADGETUP,
			      (*termination) & 0x0000FFFF,
			      gadget,
			      IntuitionBase);
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

/*********************************************************************/

/*****************
**  FindGadget	**
*****************/
struct Gadget * FindGadget (struct Window * window, int x, int y,
			    struct GadgetInfo * gi, struct IntuitionBase *IntuitionBase)
{
    struct Gadget * gadget;
    struct gpHitTest gpht;
    struct IBox ibox;
    WORD xrel, yrel;

    gpht.MethodID     = GM_HITTEST;
    gpht.gpht_GInfo   = gi;

    for (gadget = window->FirstGadget; gadget; gadget = gadget->NextGadget)
    {
    	if (!(gadget->Flags & GFLG_DISABLED))
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

	} /* if (!(gadget->Flags & GFLG_DISABLED)) */

    } /* for (gadget = window->FirstGadget; gadget; gadget = gadget->NextGadget) */

    return (gadget);

} /* FindGadget */


/*******************
**  InsideGadget  **
*******************/
BOOL InsideGadget(struct Window *win, struct Gadget *gad,
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


/*********************************************************************/

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
/*********************************************************************/
/*********************************************************************/
/*********************************************************************/
/*********************************************************************/
/*********************************************************************/
/*********************************************************************/
/*********************************************************************/
/*********************************************************************/
