/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Classes for window decor stuff, like dragbar, close etc.
    
    Lang: english
*/

/***********************************************************************************/

#include <string.h>

#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/layers.h>

#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <intuition/imageclass.h>
#include <aros/asmcall.h>

#include "gadgets.h"

#include "intuition_intern.h"
#include "inputhandler.h"
#include "inputhandler_support.h"

#undef SDEBUG
#undef DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

/***********************************************************************************/

#define G(o)  ((struct Gadget *)o)
#define EG(o) ((struct ExtGadget *)o)
#define IM(o) ((struct Image *)o)

/***********************************************************************************/

struct dragbar_data
{
    /* Current left- and topedge of moving window. Ie when the user releases
    the LMB after a windowdrag, the window's new coords will be (curleft, curtop)
    */

    LONG curleft;
    LONG curtop;

    /* The current x and y coordinates relative to curleft/curtop */
    LONG mousex;
    LONG mousey;

    /* Whether the dragframe is currently drawn or erased */
    BOOL isrendered;

    /* Used to tell GM_GOINACTIVE whether the drag was canceled or not */
    BOOL drag_canceled;

    /* Rastport to use during update */
    struct RastPort *rp;

};

/***********************************************************************************/

/* drawwindowframe is used when the user drags or resizes a window */

#define DWF_THICK_X 2
#define DWF_THICK_Y 2

/***********************************************************************************/

static void cliprectfill(struct Screen *scr, struct RastPort *rp,
    	    	    	 WORD x1, WORD y1, WORD x2, WORD y2,
			 struct IntuitionBase *IntuitionBase)
{
    WORD scrx2 = scr->Width  - 1;
    WORD scry2 = scr->Height - 1;
    
    /* Check if inside at all */
    
    if (!((x1 > scrx2) || (x2 < 0) || (y1 > scry2) || (y2 < 0)))
    {
    	if (x1 < 0) x1 = 0;
	if (y1 < 0) y1 = 0;
	if (x2 > scrx2) x2 = scrx2;
	if (y2 > scry2) y2 = scry2;
	
	/* paranoia */
	
	if ((x2 >= x1) && (y2 >= y1))
	{
	    RectFill(rp, x1, y1, x2, y2);
	}	
    }

}

/***********************************************************************************/

static void drawwindowframe(struct Screen *scr, struct RastPort *rp,
    	    	    	    WORD x1, WORD y1, WORD x2, WORD y2,
			    struct IntuitionBase *IntuitionBase)
{
    /* this checks should not be necessary, but just to be sure */

    if (x2 < x1)
    {
       /* swap x2 and x1 */
       x2 ^= x1;
       x1 ^= x2;
       x2 ^= x1;
    }

    if (y2 < y1)
    {
       /* swap y2 and y1 */
       y2 ^= y1;
       y1 ^= y2;
       y2 ^= y1;
    }
    
    if (((x2 - x1) < (DWF_THICK_X * 2)) ||
        ((y2 - y1) < (DWF_THICK_Y * 2)))
    {
    	cliprectfill(scr, rp, x1, y1, x2, y2, IntuitionBase);
    }
    else
    {
    	cliprectfill(scr, rp, x1, y1, x2, y1 + DWF_THICK_Y - 1, IntuitionBase);
	cliprectfill(scr, rp, x2 - DWF_THICK_X + 1, y1 + DWF_THICK_Y, x2, y2, IntuitionBase);
	cliprectfill(scr, rp, x1, y2 - DWF_THICK_Y + 1, x2 - DWF_THICK_X, y2, IntuitionBase);
	cliprectfill(scr, rp, x1, y1 + DWF_THICK_Y, x1 + DWF_THICK_X - 1, y2 - DWF_THICK_Y, IntuitionBase);
    }
}

/***********************************************************************************/

#define IntuitionBase ((struct IntuitionBase *)(cl)->cl_UserData)

/***********************************************************************************/

static VOID dragbar_render(Class *cl, Object *o, struct gpRender * msg)
{
    EnterFunc(bug("DragBar::Render()\n"));
    /* We will let the AROS gadgetclass test if it is safe to render */
    if ( DoSuperMethodA(cl, o, (Msg)msg) != 0)
    {
	struct DrawInfo 	*dri = msg->gpr_GInfo->gi_DrInfo;
        UWORD 			*pens = dri->dri_Pens;
	struct RastPort 	*rp = msg->gpr_RPort;
	struct IBox 		container;
	struct Window 		*win = msg->gpr_GInfo->gi_Window;
	struct TextExtent 	te;

	GetGadgetIBox(o, msg->gpr_GInfo, &container);
	
	if (container.Width <= 1 || container.Height <= 1)
	    return;
	    
	
	/* Clear the dragbar */
	
	SetAPen(rp, (win->Flags & WFLG_WINDOWACTIVE) ? 
			pens[FILLPEN] : pens[BACKGROUNDPEN]);
			
	SetDrMd(rp, JAM1);

	D(bug("Filling from (%d, %d) to (%d, %d)\n",
	    container.Left,
	    container.Top,
	    container.Left + container.Width - 1,
	    container.Top + container.Height - 1));
		
	RectFill(rp,
	    container.Left,
	    container.Top,
	    container.Left + container.Width - 1,
	    container.Top + container.Height - 2);
	    
	/* Draw a thin dark line around the bar */
	
	SetAPen(rp, pens[SHINEPEN]);
	RectFill(rp,container.Left,
		    container.Top,
		    container.Left,
		    container.Top + container.Height - 1 - ((container.Left == 0) ? 0 : 1));
	RectFill(rp,container.Left + 1,
		    container.Top,
		    container.Left + container.Width - 1,
		    container.Top);

	SetAPen(rp,pens[SHADOWPEN]);
	RectFill(rp,container.Left + container.Width - 1,
		    container.Top + 1,
		    container.Left + container.Width - 1,
		    container.Top + container.Height - 1);
	RectFill(rp,container.Left + ((container.Left == 0) ? 1 : 0),
		    container.Top + container.Height - 1,
		    container.Left + container.Width - 2,
		    container.Top + container.Height - 1);
	
	/* Render the titlebar */
	if (NULL != win->Title)
	{
	    ULONG textlen, titlelen;
	    
	    SetFont(rp, dri->dri_Font);

	    titlelen = strlen(win->Title);
	    textlen = TextFit(rp
		, win->Title
		, titlelen
		, &te
		, NULL
		, 1
		, container.Width - 6
		, container.Height);

	    SetAPen(rp, pens[(win->Flags & WFLG_WINDOWACTIVE) ? FILLTEXTPEN : TEXTPEN]);
	    Move(rp, container.Left + 3, container.Top + dri->dri_Font->tf_Baseline + 3);
	
	    Text(rp, win->Title, textlen);
	}
	
    }  /* if (allowed to render) */

    ReturnVoid("DragBar::Render");
}

/***********************************************************************************/

static IPTR dragbar_goactive(Class *cl, Object *o, struct gpInput *msg)
{
    
    IPTR retval = GMR_NOREUSE;
    
    struct InputEvent *ie = msg->gpi_IEvent;
    
    if (ie)
    {
    	/* The gadget was activated via mouse input */
	struct dragbar_data 	*data;
	struct Window 		*w;

	/* There is no point in rerendering ourseleves her, as this
	   is done by a call to RefreshWindowFrame() in the intuition inputhandler
	*/
    
	w = msg->gpi_GInfo->gi_Window;

	data = INST_DATA(cl, o);
	
	
	data->drag_canceled = FALSE;
	
	data->curleft = w->LeftEdge;
	data->curtop  = w->TopEdge;
    
	data->mousex = w->WScreen->MouseX - data->curleft;
	data->mousey = w->WScreen->MouseY - data->curtop;
	
	data->rp = CloneRastPort(&w->WScreen->RastPort);
	if (data->rp)
	{
	    /* Lock all layers while the window is dragged */
	    
    	    D(bug("locking all layers\n"));

    	    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock);
	    LockLayers(&w->WScreen->LayerInfo);
	    	    
	    SetDrMd(data->rp, COMPLEMENT);
	    
	    drawwindowframe(w->WScreen
	    	    	    , data->rp
			    , data->curleft
			    , data->curtop
			    , data->curleft + w->Width  - 1
			    , data->curtop  + w->Height - 1
			    , IntuitionBase
	    );
	    
	    data->isrendered = TRUE;
	    
	    retval = GMR_MEACTIVE;
	}
    }

    return retval;


}

/***********************************************************************************/

static IPTR dragbar_handleinput(Class *cl, Object *o, struct gpInput *msg)
{
    IPTR retval = GMR_MEACTIVE;
    struct GadgetInfo *gi = msg->gpi_GInfo;
    
    if (gi)
    {
	struct InputEvent 	*ie = msg->gpi_IEvent;
	struct dragbar_data 	*data = INST_DATA(cl, o);
	struct Window 		*w = msg->gpi_GInfo->gi_Window;
	
	switch (ie->ie_Class)
	{
	    case IECLASS_RAWMOUSE:
		switch (ie->ie_Code)
		{
		    case SELECTUP:
	    		retval = GMR_NOREUSE;
			break;


		    case IECODE_NOBUTTON: {
	    		struct Screen 	*scr = w->WScreen;
			LONG 		new_left;
			LONG 		new_top;


	    		/* Can we move to the new position, or is window at edge of display ? */
			new_left = scr->MouseX - data->mousex;
			new_top  = scr->MouseY - data->mousey;

    	    	        if (!(scr->LayerInfo.Flags & LIFLG_SUPPORTS_OFFSCREEN_LAYERS))
			{
			    if (new_left < 0)
			    {
				data->mousex += new_left;
				new_left = 0;
			    }

			    if (new_top < 0)
			    {
				data->mousey += new_top;
				new_top = 0;
			    }

			    if (new_left + w->Width > scr->Width)
			    {
				LONG correct_left;
				correct_left = scr->Width - w->Width; /* align to screen border */
				data->mousex += new_left - correct_left;
				new_left = correct_left;
			    }
			    if (new_top + w->Height > scr->Height)
			    {
				LONG correct_top;
				correct_top = scr->Height - w->Height; /* align to screen border */
				data->mousey += new_top - correct_top;
				new_top = correct_top;
			    }
    	    	    	}

			if (data->curleft != new_left || data->curtop != new_top)
			{
			    SetDrMd(data->rp, COMPLEMENT);



	    		    if (data->isrendered)
			    {
				/* Erase old frame */
				drawwindowframe(w->WScreen
				    	    	, data->rp
						, data->curleft
						, data->curtop
						, data->curleft + w->Width  - 1
						, data->curtop  + w->Height - 1
						, IntuitionBase
				);

			    }

			    data->curleft = new_left;
			    data->curtop  = new_top;

			    /* Rerender the window frame */

			    drawwindowframe(w->WScreen
			    	    	   , data->rp
					   , data->curleft
					   , data->curtop
					   , data->curleft + w->Width  - 1
					   , data->curtop  + w->Height - 1
					   , IntuitionBase
			    );

			    data->isrendered = TRUE;


			}

			retval = GMR_MEACTIVE;

			break; }

		    default:
	    		retval = GMR_REUSE;
			data->drag_canceled = TRUE;
			break;



		} /* switch (ie->ie_Code) */
	        break;
	    
	} /* switch (ie->ie_Class) */

    } /* if (gi) */
    
    return retval;
}

/***********************************************************************************/

static IPTR dragbar_goinactive(Class *cl, Object *o, struct gpGoInactive *msg)
{
    struct dragbar_data *data;
    struct Window 	*w;
    
    data = INST_DATA(cl, o);
    w = msg->gpgi_GInfo->gi_Window;
    
    /* Allways clear last drawn frame */
			    
    if (data->isrendered)
    {

	SetDrMd(data->rp, COMPLEMENT);

	/* Erase old frame */
	drawwindowframe(w->WScreen
	    	       , data->rp
		       , data->curleft
		       , data->curtop
		       , data->curleft + w->Width  - 1
		       , data->curtop  + w->Height - 1
		       , IntuitionBase
	);

    }


    if (!data->drag_canceled)
    {
	    
		
	MoveWindow(w
	    , data->curleft - w->LeftEdge	/* dx */
	    , data->curtop  - w->TopEdge	/* dy */
	);
		
    }
    data->drag_canceled = FALSE;


    /* User throught with drag operation. Unlock layesr and free
	   rastport clone
    */
    UnlockLayers(&w->WScreen->LayerInfo);
    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock);

    FreeRastPort(data->rp);
    
    return TRUE;
	
    
}

/***********************************************************************************/

/***********  Window dragbar class **********************************/

AROS_UFH3S(IPTR, dispatch_dragbarclass,
    AROS_UFHA(Class *,  cl,  A0),
    AROS_UFHA(Object *, o,   A2),
    AROS_UFHA(Msg,      msg, A1)
)
{
    AROS_USERFUNC_INIT

    IPTR retval = 0UL;
    
    EnterFunc(bug("dragbar_dispatcher(mid=%d)\n", msg->MethodID));
    switch (msg->MethodID)
    {
	case GM_RENDER:
	    dragbar_render(cl, o, (struct gpRender *)msg);
	    break;
	    
	case GM_LAYOUT:
	    break;
	    
	case GM_DOMAIN:
	    break;
	    
	case GM_GOACTIVE:
	    retval = dragbar_goactive(cl, o, (struct gpInput *)msg);
	    break;
	    
	case GM_GOINACTIVE:
	    retval = dragbar_goinactive(cl, o, (struct gpGoInactive *)msg);
	    break;
	    
	case GM_HANDLEINPUT:
	    retval = dragbar_handleinput(cl, o, (struct gpInput *)msg);
	    break;
	    
	case OM_NEW:
	    retval = DoSuperMethodA(cl, o, msg);
	    if (NULL != retval) {
	    	((struct Gadget *)retval)->GadgetType |= GTYP_SYSGADGET;
	    }
	    break;
	

	default:
	    retval = DoSuperMethodA(cl, o, msg);
	    break;
    }
    
    ReturnPtr ("dragbar_dispatcher", IPTR, retval);

    AROS_USERFUNC_EXIT
}

/***********************************************************************************/

/*********************
** The SizeButtonClass
*********************/

struct sizebutton_data
{

     /* The current width and height of the rubber band frame */
     ULONG width;
     ULONG height;
     
     /* the offset of the mouse pointer to the rubber band frame*/
     LONG mouseoffsetx;
     LONG mouseoffsety;
     
     /* Whether the dragframe is currently drawn or erased */
     BOOL isrendered;
     
     /* Used to tell GM_GOINACTIVE whether the drag was canceled or not */
     BOOL drag_canceled;
     
     /* Rastport to use during update */
     struct RastPort *rp;
     
};

/***********************************************************************************/

static IPTR sizebutton_goactive(Class *cl, Object *o, struct gpInput *msg)
{
    
    IPTR 		retval = GMR_NOREUSE;
    
    struct InputEvent 	*ie = msg->gpi_IEvent;
    
    if (ie)
    {
    	/* The gadget was activated via mouse input */
	struct sizebutton_data 	*data;
	struct Window 		*w;

	/* There is no point in rerendering ourseleves her, as this
	   is done by a call to RefreshWindowFrame() in the intuition inputhandler
	*/
    
	w = msg->gpi_GInfo->gi_Window;
    
	data = INST_DATA(cl, o);
	
	
	data->drag_canceled = FALSE;
	
	data->height = w->Height;
	data->width  = w->Width;
        
        data->mouseoffsetx = w->Width  - (w->WScreen->MouseX - w->LeftEdge);
        data->mouseoffsety = w->Height - (w->WScreen->MouseY - w->TopEdge);

	data->rp = CloneRastPort(&w->WScreen->RastPort);
	if (data->rp)
	{      
	    /* Lock all layers while the window is resized */
	    
    	    D(bug("locking all layers\n"));
	    
    	    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock);
	    LockLayers(&w->WScreen->LayerInfo);
	    
	    SetDrMd(data->rp, COMPLEMENT);
	    
	    drawwindowframe(w->WScreen
	    	    	    , data->rp
			    , w->LeftEdge
			    , w->TopEdge
			    , w->LeftEdge + data->width  - 1
			    , w->TopEdge  + data->height - 1
			    , IntuitionBase
	    );
	
	    data->isrendered = TRUE;
	    
	    retval = GMR_MEACTIVE;
	}
    }

    return retval;
    
    
}

/***********************************************************************************/

static IPTR sizebutton_handleinput(Class *cl, Object *o, struct gpInput *msg)
{
    IPTR 		retval = GMR_MEACTIVE;
    struct GadgetInfo 	*gi = msg->gpi_GInfo;
    
    if (gi)
    {
	struct InputEvent 	*ie = msg->gpi_IEvent;
	struct sizebutton_data 	*data = INST_DATA(cl, o);
	struct Window 		*w = msg->gpi_GInfo->gi_Window;
	
	switch (ie->ie_Class)
	{
	    case IECLASS_RAWMOUSE:
		switch (ie->ie_Code)
		{
		    case SELECTUP:
	    		retval = GMR_NOREUSE;
			break;


		    case IECODE_NOBUTTON: {
	    		struct Screen 	*scr = w->WScreen;
			LONG 		new_width;
			LONG 		new_height;


	    		/* Can we move to the new position, or is window at edge of display ? */
			new_width   = scr->MouseX - w->LeftEdge + data->mouseoffsetx;
			new_height  = scr->MouseY - w->TopEdge  + data->mouseoffsety;

			if (new_width < 0)
			  new_width = 1;

			if (w->MinWidth != 0 && new_width < (ULONG)w->MinWidth)
			  new_width = w->MinWidth;

			if (w->MaxWidth != 0 && new_width > (ULONG)w->MaxWidth)
			  new_width = w->MaxWidth;

			if (new_height < 0)
			  new_height = 1;

			if (w->MinHeight != 0 && new_height < (ULONG)w->MinHeight)
			  new_height = w->MinHeight;

			if (w->MaxHeight != 0 && new_height > (ULONG)w->MaxHeight)
			  new_height = w->MaxHeight;


    	    	    	if (!(w->WScreen->LayerInfo.Flags & LIFLG_SUPPORTS_OFFSCREEN_LAYERS))
			{
                	    /* limit dimensions so window fits on the screen */		
			    if (new_width + w->LeftEdge > scr->Width)
			      new_width = scr->Width - w->LeftEdge;

			    if (new_height + w->TopEdge > scr->Height)
			      new_height = scr->Height - w->TopEdge;
    	    	    	}

			if (data->height != new_height || data->width != new_width)
			{
			    SetDrMd(data->rp, COMPLEMENT);

	    		    if (data->isrendered)
			    {
				/* Erase old frame */
				drawwindowframe(w->WScreen
				    	    	, data->rp
						, w->LeftEdge
						, w->TopEdge
						, w->LeftEdge + data->width  - 1
						, w->TopEdge  + data->height - 1
						, IntuitionBase
				);

			    }

			    data->width   = new_width;
			    data->height  = new_height;

			    /* Rerender the window frame */

  			    drawwindowframe(w->WScreen
			    	    	   , data->rp
					   , w->LeftEdge
					   , w->TopEdge
					   , w->LeftEdge + data->width  - 1
					   , w->TopEdge  + data->height - 1
					   , IntuitionBase
				);

			    data->isrendered = TRUE;


			}

			retval = GMR_MEACTIVE;

			break; }

		    default:
	    		retval = GMR_REUSE;
			data->drag_canceled = TRUE;
			break;



		} /* switch (ie->ie_Code) */
		break;
	    
	    
	} /* switch (ie->ie_Class) */
	
    } /* if (gi) */
    return retval;
}

/***********************************************************************************/

static IPTR sizebutton_goinactive(Class *cl, Object *o, struct gpGoInactive *msg)
{
    struct sizebutton_data 	*data;
    struct Window 		*w;
    
    data = INST_DATA(cl, o);
    w = msg->gpgi_GInfo->gi_Window;
    
    /* Allways clear last drawn frame */
			    
    if (data->isrendered)
    {

	SetDrMd(data->rp, COMPLEMENT);

	/* Erase old frame */
	drawwindowframe(w->WScreen
	    	       , data->rp
		       , w->LeftEdge
		       , w->TopEdge
		       , w->LeftEdge + data->width  - 1
		       , w->TopEdge  + data->height - 1
		       , IntuitionBase
	);

    }
	
    if (!data->drag_canceled)
    {
	    
		
	SizeWindow(w
	    , data->width  - w->Width	/* dx */
	    , data->height - w->Height	/* dy */
	);
		
    }
    data->drag_canceled = FALSE;
		    
		
    /* User throught with drag operation. Unlock layesr and free
	   rastport clone
    */
    UnlockLayers(&w->WScreen->LayerInfo);
    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock);
    
    FreeRastPort(data->rp);
    
    return TRUE;
}


/***********************************************************************************/

/***********  Size Button class **********************************/

AROS_UFH3S(IPTR, dispatch_sizebuttonclass,
    AROS_UFHA(Class *,  cl,  A0),
    AROS_UFHA(Object *, o,   A2),
    AROS_UFHA(Msg,      msg, A1)
)
{
    AROS_USERFUNC_INIT

    IPTR retval = 0UL;

    EnterFunc(bug("sizebutton_dispatcher(mid=%d)\n", msg->MethodID));
    switch (msg->MethodID)
    {
	case GM_LAYOUT:
	    break;
	    
	case GM_DOMAIN:
	    break;
	    
	case GM_GOACTIVE:
	    retval = sizebutton_goactive(cl, o, (struct gpInput *)msg);
	    break;

	case GM_GOINACTIVE:
	    retval = sizebutton_goinactive(cl, o, (struct gpGoInactive *)msg);
	    break;
	    
	case GM_HANDLEINPUT:
	    retval = sizebutton_handleinput(cl, o, (struct gpInput *)msg);
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, o, msg);
	    break;
    }

    ReturnPtr ("sizebutton_dispatcher", IPTR, retval);

    AROS_USERFUNC_EXIT
}

/***********************************************************************************/

#undef IntuitionBase

/***********************************************************************************/

struct IClass *InitDragBarClass (struct IntuitionBase * IntuitionBase)
{
    struct IClass *cl = NULL;

    /* This is the code to make the dragbarclass...
    */
    if ( (cl = MakeClass(NULL, GADGETCLASS, NULL, sizeof (struct dragbar_data), 0)) )
    {
	cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_dragbarclass);
	cl->cl_Dispatcher.h_SubEntry = NULL;
	cl->cl_UserData 	     = (IPTR)IntuitionBase;

    }

    return (cl);
}

/***********************************************************************************/

struct IClass *InitSizeButtonClass (struct IntuitionBase * IntuitionBase)
{
    struct IClass *cl = NULL;

    /* This is the code to make the dragbarclass...
    */
    if ( (cl = MakeClass(NULL, GADGETCLASS, NULL, sizeof (struct sizebutton_data), 0)) )
    {
	cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_sizebuttonclass);
	cl->cl_Dispatcher.h_SubEntry = NULL;
	cl->cl_UserData 	     = (IPTR)IntuitionBase;
    }

    return (cl);
}

/***********************************************************************************/

