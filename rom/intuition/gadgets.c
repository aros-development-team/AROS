/*
    (C) 1995-98 AROS - The Amiga Research OS
    $Id$

    Desc: Common routines for Gadgets
    Lang: english
*/
#include "intuition_intern.h"
#include "gadgets.h"
#include <exec/types.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <intuition/cghooks.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <graphics/rastport.h>
#include <graphics/text.h>

#define EG(o) ((struct ExtGadget *)o)
#define IM(o) ((struct Image *)o)

/* print the label of a gadget object */
#warning FIXME: printgadgetlabel() has to be reworked!
void printgadgetlabel(Class *cl, Object *o, struct gpRender *msg)
{
    struct RastPort *rp = msg->gpr_RPort;
    struct IBox container;
    UWORD *pens = msg->gpr_GInfo->gi_DrInfo->dri_Pens;

    GetGadgetIBox(o, msg->gpr_GInfo, &container);
    
    switch (EG(o)->Flags & GFLG_LABELMASK)
    {
    case GFLG_LABELITEXT: {
	/* struct TextExtent te; */

	/* Georg Steger: ITexts are not to be centered 
	
	   TextExtent(rp, EG(o)->GadgetText->IText,
		   strlen(EG(o)->GadgetText->IText), &te); */
 
        PrintIText(rp,
	    EG(o)->GadgetText,
	    container.Left /* + (container.Width  - te.te_Width ) / 2 */,
	    container.Top  /* + (container.Height - te.te_Height) / 2 */
        );
	break; }

    case GFLG_LABELSTRING:
        if(EG(o)->GadgetText != NULL)
	{
	    ULONG len = strlen ((STRPTR) EG(o)->GadgetText);

	    if (len > 0UL)
	    {
	        ULONG x;
		ULONG y;

		x = container.Left + (container.Width / 2);
		x -= LabelWidth (rp,
		    (STRPTR)EG(o)->GadgetText, len, IntuitionBase) / 2;
		y = container.Top + (container.Height / 2) +
		    rp->Font->tf_Baseline;
		y -= rp->Font->tf_YSize / 2;
		SetAPen (rp, pens[TEXTPEN]);
		Move (rp, x, y);
		RenderLabel (rp,
		    (STRPTR) EG(o)->GadgetText, len,
		    IntuitionBase);
	    }
	}
	break;

    case GFLG_LABELIMAGE:
        {
            /* center image position, we assume image top and left is 0 */
            ULONG x = container.Left + ((container.Width / 2) -
		(IM(EG(o)->GadgetText)->Width / 2));
            ULONG y = container.Top + ((container.Height / 2) -
	        (IM(EG(o)->GadgetText)->Height / 2));

            DrawImageState(rp,
	        IM(EG(o)->GadgetText),
		x, y,
		((EG(o)->Flags & GFLG_SELECTED) ? IDS_SELECTED : IDS_NORMAL ),
		msg->gpr_GInfo->gi_DrInfo);
        }
        break;
    }
}

/* Calculate the size of the Bounding Box of the gadget */
void CalcBBox (struct Window * window, struct Gadget * gadget,
	struct BBox * bbox)
{
   /* 

    #define ADDREL(flag,field)  ((gadget->Flags & (flag)) ? window->field : 0)

    bbox->Left	 = ADDREL(GFLG_RELRIGHT,Width-1)   + gadget->LeftEdge;
    bbox->Top	 = ADDREL(GFLG_RELBOTTOM,Height-1) + gadget->TopEdge;
    bbox->Width  = ADDREL(GFLG_RELWIDTH,Width)     + gadget->Width;
    bbox->Height = ADDREL(GFLG_RELHEIGHT,Height)   + gadget->Height;
    
    */
    
   GetDomGadgetIBox(gadget, window->WScreen, window, NULL, (struct IBox *)bbox);
   
} /* CalcBBox */

/* Figure out the size of the gadget rectangle, taking relative
 * positioning into account.
 */
VOID GetGadgetIBox(Object *o, struct GadgetInfo *gi, struct IBox *ibox)
{
    ibox->Left	 = EG(o)->LeftEdge;
    ibox->Top	 = EG(o)->TopEdge;
    ibox->Width  = EG(o)->Width;
    ibox->Height = EG(o)->Height;

    if (gi)
    {
	if (EG(o)->Flags & GFLG_RELRIGHT)
	    ibox->Left	 += gi->gi_Domain.Width - 1;

	if (EG(o)->Flags & GFLG_RELBOTTOM)
	    ibox->Top	 += gi->gi_Domain.Height - 1;

	if (EG(o)->Flags & GFLG_RELWIDTH)
	    ibox->Width  += gi->gi_Domain.Width;

	if (EG(o)->Flags & GFLG_RELHEIGHT)
	    ibox->Height += gi->gi_Domain.Height;
    }
    
}

ULONG LabelWidth (struct RastPort * rp, STRPTR label, ULONG len,
		struct IntuitionBase * IntuitionBase)
{
    ULONG totalwidth, uscorewidth;

    totalwidth	= TextLength (rp, label, len);
    uscorewidth = TextLength (rp, "_", 1);

    while (len && *label)
    {
	if (*label == '_')
	    totalwidth -= uscorewidth;

	label ++;
	len --;
    }

    return totalwidth;
}

void RenderLabel (struct RastPort * rp, STRPTR label, ULONG len,
		struct IntuitionBase * IntuitionBase)
{
    ULONG renderlen;
    ULONG uscorewidth;

    while (*label)
    {
	renderlen = 0;

	while (label[renderlen] && label[renderlen] != '_')
	    renderlen ++;

	Text (rp, label, renderlen);

	label += renderlen;

	if (*label) /* '_' ? */
	{
	    WORD cx, cy;

	    label ++; /* Skip */

	    uscorewidth = TextLength (rp, label, 1);

	    cx = rp->cp_x;
	    cy = rp->cp_y;

	    Move (rp, cx, cy+2);
	    Draw (rp, cx+uscorewidth-1, cy+2);
	    Move (rp, cx, cy);
	}
    }
}


VOID drawrect(struct RastPort *rp
	, WORD x1, WORD y1
	, WORD x2, WORD y2
	, struct IntuitionBase *IntuitionBase)
{
    Move(rp, x1, y1);
    
    /* We RectFill() because it is generally faster than Draw()
       (Draw() uses Set/GetPixel() while RectFill() can do higherlevel
       clipping. Also it is MUCH faster in the x11gfx hidd.
    */
       
    RectFill(rp, x1, y1, x2 - 1, y1);	/* Top		*/
    RectFill(rp, x2, y1, x2, y2 - 1);	/* Right	*/
    RectFill(rp, x1 + 1, y2, x2, y2);	/* Bottom	*/
    RectFill(rp, x1, y1 + 1, x1, y2);	/* Left		*/
    
    return;
}

void GetGadgetDomain(struct Gadget *gad, struct Screen *scr, struct Window *win,
                     struct Requester *req, struct IBox *box)
{
    switch (gad->GadgetType & (GTYP_GADGETTYPE & ~GTYP_SYSGADGET))
    {
	case GTYP_SCRGADGET:
	    box->Left	= 0;
	    box->Top	= 0;
	    box->Width  = scr->Width;
	    box->Height = scr->Height;

	    break;

	case GTYP_GZZGADGET:
	    /* stegerg: this means gadget is in window border! */

	    box->Left	= 0;
	    box->Top	= 0;
	    box->Width  = win->Width;
	    box->Height = win->Height;

	    break;

	case GTYP_REQGADGET:
	    box->Left	= req->LeftEdge;
	    box->Top	= req->TopEdge;
	    box->Width  = req->Width;
	    box->Height = req->Height;

	    break;

	default:
	    if (win->Flags & WFLG_GIMMEZEROZERO)
	    {
		/* stegerg: domain.left and domain.top must not be added
		   to gadget position when it is rendered, because gadgets
		   in the innerlayer of a gzz gadget are already shifted
		   thanks to the innerlayer. */

		box->Left   = win->BorderLeft;
		box->Top    = win->BorderTop;

		box->Width  = win->Width - win->BorderLeft - win->BorderRight;
		box->Height = win->Height - win->BorderTop - win->BorderBottom;
		
	    } else {		    	
		box->Left   = 0;
		box->Top    = 0;
		box->Width  = win->Width;
		box->Height = win->Height;
		
	    }

	    break;

    } /* switch (gadgettype) */
}

WORD GetGadgetLeft(struct Gadget *gad, struct Screen *scr, struct Window *win, struct Requester *req)
{
    struct IBox box;

    GetGadgetDomain(gad, scr, win, req, &box);
    
    return gad->LeftEdge + ADDREL(gad, GFLG_RELRIGHT, (&box), Width - 1);
}

WORD GetGadgetTop(struct Gadget *gad, struct Screen *scr, struct Window *win, struct Requester *req)
{
    struct IBox box;

    GetGadgetDomain(gad, scr, win, req, &box);
    
    return gad->TopEdge + ADDREL(gad, GFLG_RELBOTTOM, (&box), Height - 1);
}

WORD GetGadgetWidth(struct Gadget *gad, struct Screen *scr, struct Window *win, struct Requester *req)
{
    struct IBox box;

    GetGadgetDomain(gad, scr, win, req, &box);
    
    return gad->Width + ADDREL(gad, GFLG_RELWIDTH, (&box), Width);
}

WORD GetGadgetHeight(struct Gadget *gad, struct Screen *scr, struct Window *win, struct Requester *req)
{
    struct IBox box;

    GetGadgetDomain(gad, scr, win, req, &box);
    
    return gad->Height + ADDREL(gad, GFLG_RELHEIGHT, (&box), Height);
}

/* gadget box in screen coords */
void GetScrGadgetIBox(struct Gadget *gad, struct Screen *scr, struct Window *win,
		      struct Requester *req, struct IBox *box)
{
    struct IBox domain;
    
    GetGadgetDomain(gad, scr, win, req, &domain);
    
    if (req)
    {
    	/* leftedge, topedge members of window and requester
	   are on the same offset */
	   
    	win = (struct Window *)req;
    }
   
    box->Left = domain.Left +
    		gad->LeftEdge + ADDREL(gad, GFLG_RELRIGHT, (&domain), Width - 1) +
		(win ? win->LeftEdge : 0);
		
    box->Top  = domain.Top +
    		gad->TopEdge + ADDREL(gad, GFLG_RELBOTTOM, (&domain), Height - 1) +
		(win ? win->TopEdge : 0);

    box->Width = gad->Width + ADDREL(gad, GFLG_RELWIDTH, (&domain), Width);
 
    box->Height = gad->Height + ADDREL(gad, GFLG_RELHEIGHT, (&domain), Height);
}

/* gadget box relative to upper left window edge */
void GetWinGadgetIBox(struct Gadget *gad, struct Screen *scr, struct Window *win,
		      struct Requester *req, struct IBox *box)
{
    GetScrGadgetIBox(gad, scr, win, req, box);
    
    if (req)
    {
        /* leftedge, topedge members of window and requester
	   are on the same offset */
	   
    	win = (struct Window *)req;
    }
    
    if (win)
    {
    	box->Left -= win->LeftEdge;
    	box->Top  -= win->TopEdge;
    }
}

/* gadget box in domain coords */
void GetDomGadgetIBox(struct Gadget *gad, struct Screen *scr, struct Window *win,
		      struct Requester *req, struct IBox *box)
{
    struct IBox domain;
    
    GetWinGadgetIBox(gad, scr, win, req, box);
    GetGadgetDomain(gad, scr, win, req, &domain);
    
    box->Left -= domain.Left;
    box->Top  -= domain.Top;
}



/* gadget bounds in screen coords */
void GetScrGadgetBounds(struct Gadget *gad, struct Screen *scr, struct Window *win,
		        struct Requester *req, struct IBox *box)
{
    struct IBox domain;
    
    if (gad->Flags & GFLG_EXTENDED)
    {
        if (EG(gad)->MoreFlags & GMORE_BOUNDS)
	{
	    GetGadgetDomain(gad, scr, win, req, &domain);

	    if (req)
	    {
    		/* leftedge, topedge members of window and requester
		   are on the same offset */

    		win = (struct Window *)req;
	    }

	    box->Left = domain.Left +
    			EG(gad)->BoundsLeftEdge + ADDREL(gad, GFLG_RELRIGHT, (&domain), Width - 1) +
			(win ? win->LeftEdge : 0);

	    box->Top  = domain.Top +
    			EG(gad)->BoundsTopEdge + ADDREL(gad, GFLG_RELBOTTOM, (&domain), Height - 1) +
			(win ? win->TopEdge : 0);

	    box->Width = EG(gad)->BoundsWidth + ADDREL(gad, GFLG_RELWIDTH, (&domain), Width);

	    box->Height = EG(gad)->BoundsHeight + ADDREL(gad, GFLG_RELHEIGHT, (&domain), Height);
	    
	    return;

	} /* if (gad->MoreFlags & GMORE_BOUNDS) */
	
    } /* if (gad->Flags & GFLG_EXTENDED) */
    
    /* if gadget does not have bounds return box */
    
    GetScrGadgetIBox(gad, scr, win, req, box);
}

/* gadget bounds relative to upper left window edge */
void GetWinGadgetBounds(struct Gadget *gad, struct Screen *scr, struct Window *win,
		        struct Requester *req, struct IBox *box)
{
    GetScrGadgetBounds(gad, scr, win, req, box);
    
    if (req)
    {
        /* leftedge, topedge members of window and requester
	   are on the same offset */
	   
    	win = (struct Window *)req;
    }
    
    if (win)
    {
    	box->Left -= win->LeftEdge;
    	box->Top  -= win->TopEdge;
    }
}

/* gadget bounds in domain coords */
void GetDomGadgetBounds(struct Gadget *gad, struct Screen *scr, struct Window *win,
		        struct Requester *req, struct IBox *box)
{
    struct IBox domain;
    
    GetWinGadgetBounds(gad, scr, win, req, box);
    GetGadgetDomain(gad, scr, win, req, &domain);
    
    box->Left -= domain.Left;
    box->Top  -= domain.Top;
}

void EraseRelGadgetArea(struct Window *win, struct IntuitionBase *IntuitionBase)
{
    struct Gadget *gad;
    struct Region *old_clipregion;
    WORD old_scroll_x, old_scroll_y;
    struct IBox box;
    
    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock);

    LockLayerRom(win->WLayer);

    old_scroll_x = win->WLayer->Scroll_X;
    old_scroll_y = win->WLayer->Scroll_Y;
    old_clipregion = InstallClipRegion(win->WLayer, NULL);

    gad = win->FirstGadget;
    while(gad)
    {
	/* no eraserect for gadgets in the border area since
	   refreshwindowframe already "erases" the whole border
	   area */

	if (!IS_BORDER_GADGET(gad))
	{
	    if (gad->Flags & (GFLG_RELRIGHT | GFLG_RELBOTTOM |
			      GFLG_RELWIDTH | GFLG_RELHEIGHT | GFLG_RELSPECIAL))
	    {
		GetDomGadgetBounds(gad, win->WScreen, win, NULL, &box);
		EraseRect(win->RPort, box.Left,
		    		      box.Top,
				      box.Left + box.Width - 1,
				      box.Top + box.Height -1);
	    }
	} 
	gad = gad->NextGadget;
    }

    InstallClipRegion(win->WLayer, old_clipregion);
    win->WLayer->Scroll_X = old_scroll_x;
    win->WLayer->Scroll_Y = old_scroll_y;

    UnlockLayerRom(win->WLayer);

    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock);    
}

