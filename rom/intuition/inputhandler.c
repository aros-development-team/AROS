#include <proto/exec.h>
#include <proto/boopsi.h>
#include <proto/intuition.h>
#include <proto/alib.h>
#include <exec/memory.h>
#include <exec/interrupts.h>
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

#define DEBUG 0
#include <aros/debug.h>

#define ADDREL(gad,flag,w,field) ((gad->Flags & (flag)) ? w->field : 0)
#define GetLeft(gad,w)           (ADDREL(gad,GFLG_RELRIGHT,w,Width)   + gad->LeftEdge)
#define GetTop(gad,w)            (ADDREL(gad,GFLG_RELBOTTOM,w,Height) + gad->TopEdge)
#define GetWidth(gad,w)          (ADDREL(gad,GFLG_RELWIDTH,w,Width)   + gad->Width)
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
    gpht.gpht_Mouse.X = x;
    gpht.gpht_Mouse.Y = y;

    for (gadget=window->FirstGadget; gadget; gadget=gadget->NextGadget)
    {
	if ((gadget->GadgetType & GTYP_GTYPEMASK) != GTYP_CUSTOMGADGET)
	{
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
	    if (DoMethodA ((Object *)gadget, (Msg)&gpht) == GMR_GADGETHIT)
		break;
	}
    }

    return (gadget);
} /* FindGadget */




/************************
**  IntuiInputHandler  **
************************/
AROS_UFH3(struct InputEvent *, IntuiInputHandler,
    AROS_UFHA(struct InputEvent *,      oldchain,       A0),
    AROS_UFHA(struct IIHData *,         iihdata,        A1),
    AROS_UFHA(struct Window *,          w,              A2)
)
{
    struct InputEvent	*ie;
    struct IntuiMessage *im = NULL;
    struct Screen	* screen;
    struct Gadget *gadget = iihdata->ActiveGadget;
    struct IntuitionBase *IntuitionBase = iihdata->IntuitionBase;
    ULONG  lock;
    UWORD wait = 0;
    char *ptr = NULL;
    WORD mpos_x = iihdata->LastMouseX, mpos_y = iihdata->LastMouseY;
    struct GadgetInfo stackgi, *gi = &stackgi;
    BOOL reuse_event = FALSE;

    for (ie = oldchain; ie; ie = ((reuse_event) ? ie : ie->ie_NextEvent))
    {
	reuse_event = FALSE;
	ptr = NULL;

	/* If the last InnputEvent was swallowed, we can reuse the IntuiMessage */
	if (!im)
	{
	    im = AllocMem (sizeof (struct IntuiMessage), MEMF_CLEAR);
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
	    im->Code	= ie->ie_Code;
	    im->MouseX	= ie->ie_X;
	    im->MouseY	= ie->ie_Y;

	    ptr = "RAWMOUSE";

	    switch (ie->ie_Code)
	    {
	    case SELECTDOWN: {
		BOOL new_gadget = FALSE;

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
			HandlePropSelectDown(gadget, w, NULL, ie->ie_X, ie->ie_Y, IntuitionBase);
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
			gpi.gpi_Mouse.X = ie->ie_X;
			gpi.gpi_Mouse.Y = ie->ie_Y;
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
			gpi.gpi_Mouse.X = ie->ie_X;
			gpi.gpi_Mouse.Y = ie->ie_Y;
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
		ptr = "MOUSEMOVE";
		iihdata->LastMouseX = ie->ie_X;
		iihdata->LastMouseY = ie->ie_Y;


		/* Check if there is already a MOUSEMOVE in the msg queue
		** of the task
		*/
		msg = (struct IntuiMessage *)w->UserPort->mp_MsgList.lh_Head;

		Forbid ();

		while ((succ = (struct IntuiMessage *)msg->ExecMessage.mn_Node.ln_Succ))
		{
		    if (msg->Class == IDCMP_MOUSEMOVE)
		    {
			/* TODO allow a number of such messages */
			break;
		    }

		    msg = succ;
		}

		Permit ();

		/* If there is, don't add another one */
		if (succ)
		    break;


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

	    break; }

	    } /* switch (im->im_Code)  (what button was pressed ?) */
	    break;



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

	 if (im->Class)
	 {
	    if ((im->Class & w->IDCMPFlags) && w->UserPort)
	    {
		im->ExecMessage.mn_ReplyPort = iihdata->IntuiReplyPort;

		lock = LockIBase (0L);

		w->MoreFlags &= ~EWFLG_DELAYCLOSE;

		if (w->MoreFlags & EWFLG_CLOSEWINDOW)
		    CloseWindow (w);
		else
		{
		    PutMsg (w->UserPort, (struct Message *)im);
		    im = NULL;
		    wait ++;
		}

		UnlockIBase (lock);
	    }
	    else
		im->Class = 0;
	}

    } /* for (each event in the chain) */


    iihdata->ActiveGadget = gadget;

    /* If IntuiMessages has been swallowed (im->Class = 0), there is a free IntuiMessage
    ** struct (eg. not sent to the apps messageport),
    ** and we must free it here
    */

    if (im)
    {
	FreeMem (im, sizeof (struct IntuiMessage));
	im = NULL;
    }

    /* Wait for the apps to reply to the sent IntuiMessage.
    ** Each time we wake up we will probably only have receive
    ** one message. This is because input.device run on higher pri than apps,
    ** and will therefore be woke up immediately when an app
    ** calls ReplyMsg().
    */

    D(bug("Going into waitloop	for reply from app\n"));
    while (wait)
    {
	Wait (1L << iihdata->IntuiReplyPort->mp_SigBit);
    
       D(bug("Returned from wait\n"));

	/* Empty port */
	while ((im = (struct IntuiMessage *)GetMsg (iihdata->IntuiReplyPort)))
	{
	    FreeMem (im, sizeof (struct IntuiMessage));
	    wait --;
	}
    }

    D(bug("Outside waitloop\n"));
    return (oldchain);
}
