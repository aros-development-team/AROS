/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Code for PROP Gadgets
    Lang: english
*/
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <graphics/gfxmacros.h>
#include <intuition/cghooks.h>
#include <intuition/intuition.h>
#include <intuition/imageclass.h>
#include "intuition_intern.h"
#include "propgadgets.h"
#include "gadgets.h"

#undef DEBUG
#define DEBUG 0
#	include <aros/debug.h>

static WORD clickoffset_x, clickoffset_y;

VOID HandlePropSelectDown
(
     struct Gadget	*gadget,
     struct Window	*w,
     struct Requester	*req,
     UWORD		mouse_x,
     UWORD		mouse_y,
     struct IntuitionBase *IntuitionBase
)
{


    struct BBox knob;
    struct PropInfo * pi;
    UWORD dx, dy, flags;

    pi = (struct PropInfo *)gadget->SpecialInfo;
    
    if (!pi)
	return;

    CalcBBox (w, gadget, &knob);
 
    /* This func gets mouse coords relative to gadget box */
    
    mouse_x += knob.Left;
    mouse_y += knob.Top;
    
    if (!CalcKnobSize (gadget, &knob))
	return;

    clickoffset_x = mouse_x - knob.Left;
    clickoffset_y = mouse_y - knob.Top;
    
    dx = pi->HorizPot;
    dy = pi->VertPot;

    if (pi->Flags & FREEHORIZ)
    {
	if (mouse_x < knob.Left)
	{
	    if (dx > pi->HPotRes)
		dx -= pi->HPotRes;
	    else
		dx = 0;
	}
	else if (mouse_x >= knob.Left + knob.Width)
	{
	    if (dx < MAXPOT - pi->HPotRes)
		dx += pi->HPotRes;
	    else
		dx = MAXPOT;
	}
    }

    if (pi->Flags & FREEVERT)
    {
	if (mouse_y < knob.Top)
	{
	    if (dy > pi->VPotRes)
		dy -= pi->VPotRes;
	    else
		dy = 0;
	}
	else if (mouse_y >= knob.Top + knob.Height)
	{
	    if (dy < MAXPOT - pi->VPotRes)
		dy += pi->VPotRes;
	    else
		dy = MAXPOT;
	}
    }

    flags = pi->Flags;

   if (mouse_x >= knob.Left
	&& mouse_y >= knob.Top
	&& mouse_x < knob.Left + knob.Width
	&& mouse_y < knob.Top + knob.Height
    )
	flags |= KNOBHIT;
    else
	flags &= ~KNOBHIT;

    gadget->Flags |= GFLG_SELECTED;

    D(bug("New HPot: %d, new VPot: %d\n", dx, dy));
    
    NewModifyProp (gadget
	, w
	, NULL
	, flags
	, dx
	, dy
	, pi->HorizBody
	, pi->VertBody
	, 1
    );
    
    return;
}
     
VOID HandlePropSelectUp
(
    struct Gadget	*gadget,
    struct Window	*w,
    struct Requester	*req,
    struct IntuitionBase *IntuitionBase
)
{
    struct PropInfo * pi;

    pi = (struct PropInfo *)gadget->SpecialInfo;

    gadget->Flags &= ~GFLG_SELECTED;

    if (pi)
	NewModifyProp (gadget
	    , w
	    , NULL
	    , pi->Flags &= ~KNOBHIT
	    , pi->HorizPot
	    , pi->VertPot
	    , pi->HorizBody
	    , pi->VertBody
	    , 1
	);

    return;
}
    
VOID HandlePropMouseMove
(
    struct Gadget	*gadget,
    struct Window	*w,
    struct Requester	*req,
    LONG		dx,
    LONG		dy,
    struct IntuitionBase *IntuitionBase
)
{
   struct BBox knob;
   struct PropInfo * pi;

   pi = (struct PropInfo *)gadget->SpecialInfo;

   /* Has propinfo and the mouse was over the knob */
   if (pi && (pi->Flags & KNOBHIT))
   {
	CalcBBox (w, gadget, &knob);

	if (!CalcKnobSize (gadget, &knob))
	    return;


	/* Move the knob the same amount, ie.
	knob.Left += dx; knob.Top += dy;

	knob.Left = knob.Left
	+ (pi->CWidth - knob.Width)
	* pi->HorizPot / MAXPOT;

	ie. dx = (pi->CWidth - knob.Width)
	* pi->HorizPot / MAXPOT;

	or

	pi->HorizPot = (dx * MAXPOT) /
	(pi->CWidth - knob.Width);
	*/
	
	/* stegerg: dx and dy are not delta values
	            anymore but relative to gadget
		    box */
		    
	dx = dx - clickoffset_x;
	dy = dy - clickoffset_y;
	
	if (pi->Flags & FREEHORIZ
	    && pi->CWidth != knob.Width)
	{

	    dx = (dx * MAXPOT) / (pi->CWidth - knob.Width);
	    if (dx < 0)
	    {
	    	dx = 0;
	    } else if (dx > MAXPOT) {
	    	dx = MAXPOT;
	    }

	} /* FREEHORIZ */

	if (pi->Flags & FREEVERT
	    && pi->CHeight != knob.Height)
	{
	    dy = (dy * MAXPOT) / (pi->CHeight - knob.Height);

	    if (dy < 0)
	    {
	    	dy = 0;
	    } else if (dy > MAXPOT) {
	    	dy = MAXPOT;
	    }

	} /* FREEVERT */

	if ( ((pi->Flags & FREEHORIZ) && (dx != pi->HorizPot)) ||
	     ((pi->Flags & FREEVERT)  && (dy != pi->VertPot)) )
	
	{     
	    NewModifyProp (gadget
		, w
		, NULL
		, pi->Flags
		, dx
		, dy
		, pi->HorizBody
		, pi->VertBody
		, 1
		);
	}
	
    } /* Has PropInfo and Mouse is over knob */
	
    return;
}
    
int CalcKnobSize (struct Gadget * propGadget, struct BBox * knobbox)
{
    struct PropInfo * pi;
    WORD x, y;
    pi = (struct PropInfo *)propGadget->SpecialInfo;

    if (pi->Flags & PROPBORDERLESS)
    {
	pi->LeftBorder = 0;
	pi->TopBorder  = 0;
    }
    else
    {
    	if (pi->Flags & PROPNEWLOOK)
	{
	    x = y = 1;
	} else {
	    x = y = 2;
	}
	knobbox->Left += x;
	knobbox->Top += y;
	knobbox->Width -= x * 2;
	knobbox->Height -= y * 2;
	pi->LeftBorder = x;
	pi->TopBorder  = y;
    }

    pi->CWidth	   = knobbox->Width;
    pi->CHeight    = knobbox->Height;

    /*
      if (knobbox->Width < KNOBHMIN || knobbox->Height < KNOBVMIN)
      return FALSE;
      */

    if (pi->Flags & FREEHORIZ)
    {
	knobbox->Width = pi->CWidth * pi->HorizBody / MAXBODY;
        if (knobbox->Width < KNOBHMIN) knobbox->Width = KNOBHMIN;
	
	knobbox->Left = knobbox->Left + (pi->CWidth - knobbox->Width)
		* pi->HorizPot / MAXPOT;

	if (pi->HorizBody)
	{
	    if (pi->HorizBody < MAXBODY/2)
		pi->HPotRes = MAXPOT * 32768 / ((MAXBODY * 32768 / pi->HorizBody) - 32768);
	    else
		pi->HPotRes = MAXPOT;
	}
	else
	    pi->HPotRes = 1;
    }

    if (pi->Flags & FREEVERT)
    {
	knobbox->Height = pi->CHeight * pi->VertBody / MAXBODY;
        if (knobbox->Height < KNOBVMIN) knobbox->Height = KNOBVMIN;
	
	knobbox->Top = knobbox->Top + (pi->CHeight - knobbox->Height)
		* pi->VertPot / MAXPOT;

	if (pi->VertBody)
	{
	    if (pi->VertBody < MAXBODY/2)
		pi->VPotRes = MAXPOT * 32768 / ((MAXBODY * 32768 / pi->VertBody) - 32768);
	    else
		pi->VPotRes = MAXPOT;
	}
	else
	    pi->VPotRes = 1;
    }

    return TRUE;
} /* CalcKnobSize */


void RefreshPropGadget (struct Gadget * gadget, struct Window * window,
	struct IntuitionBase * IntuitionBase)
{
    struct PropInfo * pi;
    struct DrawInfo * dri;
    struct GadgetInfo gi;
    struct RastPort * rp;
    struct BBox bbox, kbox;
    
    D(bug("RefreshPropGadget(gad=%p, win=%s)\n", gadget, window->Title));
    
    if ((dri = GetScreenDrawInfo(window->WScreen)))
    {
	CalcBBox (window, gadget, &bbox);
	kbox = bbox;
	
	if (bbox.Width <= 0 || bbox.Height <= 0)
	    return;

        SET_GI_RPORT(&gi, window, gadget);
	gi.gi_Layer = gi.gi_RastPort->Layer;
	
	if ((rp = ObtainGIRPort(&gi)))
	{
	    pi = (struct PropInfo *)gadget->SpecialInfo;

	    if (!pi)
		return;

	    SetDrMd (rp, JAM2);

	    if (!(pi->Flags & PROPBORDERLESS))
	    {
		SetAPen(rp,dri->dri_Pens[SHADOWPEN]);
		drawrect(rp,bbox.Left,
	    		    bbox.Top,
			    bbox.Left + bbox.Width - 1,
			    bbox.Top + bbox.Height - 1,
			    IntuitionBase);

		bbox.Left++;
		bbox.Top++;
		bbox.Width -= 2;
		bbox.Height -= 2;
		if (!(pi->Flags & PROPNEWLOOK))
		{
	    	    bbox.Left++;
		    bbox.Top++;
		    bbox.Width -= 2;
		    bbox.Height -= 2;
		}
	    }

	    if (pi->Flags & PROPNEWLOOK)
	    {
		UWORD pattern[] = {0x5555,0xAAAA};

		SetAfPt(rp, pattern, 1);
		SetAPen(rp, dri->dri_Pens[SHADOWPEN]);
		SetBPen(rp, dri->dri_Pens[IS_BORDER_GADGET(gadget) ?
					((window->Flags & WFLG_WINDOWACTIVE) ? FILLPEN : BACKGROUNDPEN)
							  	             : BACKGROUNDPEN]);
		RectFill(rp, bbox.Left,
	    		     bbox.Top,
			     bbox.Left + bbox.Width - 1,
			     bbox.Top + bbox.Height - 1);
		SetAfPt(rp, 0, 0);

	    } else {
		SetAPen(rp, dri->dri_Pens[BACKGROUNDPEN]);
		RectFill(rp, bbox.Left,
	    		     bbox.Top,
			     bbox.Left + bbox.Width - 1,
			     bbox.Top + bbox.Height - 1);
	    }
	    ReleaseGIRPort(rp);
	    
	} /* if ((rp = ObtainGIRPort(&gi))) */

	if (CalcKnobSize (gadget, &kbox))
	{
	    RefreshPropGadgetKnob (gadget, NULL, &kbox, window, IntuitionBase);
	}
	
	if (gadget->Flags & GFLG_DISABLED)
	{
	    CalcBBox (window, gadget, &bbox);

	    if ((rp = ObtainGIRPort(&gi)))
	    {
		RenderDisabledPattern(rp, dri, bbox.Left,
	    				       bbox.Top,
					       bbox.Left + bbox.Width - 1,
					       bbox.Top + bbox.Height - 1,
					       IntuitionBase);
					  
	        ReleaseGIRPort(rp);
	    }
	}
	
	FreeScreenDrawInfo(window->WScreen, dri);
	
    } /* if ((dri = GetScreenDrawInfo(window->WScreen))) */
    
    ReturnVoid("RefreshPropGadget");
} /* RefreshPropGadget */


void RefreshPropGadgetKnob (struct Gadget * gadget, struct BBox * clear,
	struct BBox * knob, struct Window * window,
	struct IntuitionBase * IntuitionBase)
{
    struct DrawInfo * dri;
    struct RastPort * rp;
    struct PropInfo * pi;
    struct GadgetInfo gi;
    
    UWORD flags;
    
    D(bug("RefresPropGadgetKnob(flags=%d, clear=%p, knob = %p, win=%s)\n",
    	flags, clear, knob, window->Title));

    pi = (struct PropInfo *)gadget->SpecialInfo;
    flags = pi->Flags;
    
    if ((dri = GetScreenDrawInfo(window->WScreen)))
    {
    	SET_GI_RPORT(&gi, window, gadget),
	gi.gi_Layer = gi.gi_RastPort->Layer;
	
    	if ((rp = ObtainGIRPort(&gi)))
	{		    
	    SetDrMd (rp, JAM2);

	    if (clear)
	    {
		struct Rectangle a, b, clearrects[4];
		WORD		 i, nrects;
		
		a.MinX = clear->Left;
		a.MinY = clear->Top;
		a.MaxX = clear->Left + clear->Width - 1;
		a.MaxY = clear->Top + clear->Height - 1;
		
		b.MinX = knob->Left;
		b.MinY = knob->Top;
		b.MaxX = knob->Left + knob->Width - 1;
		b.MaxY = knob->Top + knob->Height - 1;
				
		nrects = SubtractRectFromRect(&a, &b, clearrects);
		
		if (pi->Flags & PROPNEWLOOK)
		{
		    UWORD pattern[] = {0x5555,0xAAAA};

		    SetAfPt(rp, pattern, 1);
		    SetAPen(rp, dri->dri_Pens[SHADOWPEN]);
		    SetBPen(rp, dri->dri_Pens[IS_BORDER_GADGET(gadget) ?
		    			((window->Flags & WFLG_WINDOWACTIVE) ? FILLPEN : BACKGROUNDPEN)
							  		     : BACKGROUNDPEN]);
		} else {
		    SetAPen(rp, dri->dri_Pens[BACKGROUNDPEN]);
		}
		
		/*kprintf("\n=== oldknob = %d,%d-%d,%d   newknob = %d,%d-%d,%d\n",
			a.MinX,
			a.MinY,
			a.MaxX,
			a.MaxY,
			b.MinX,
			b.MinY,
			b.MaxX,
			b.MaxY);*/
			
		for(i = 0; i < nrects; i++)
		{
		    /*kprintf("=== clearing %d,%d - %d,%d\n",
		    clearrects[i].MinX,clearrects[i].MinY,clearrects[i].MaxX,clearrects[i].MaxY);*/
		    
		    RectFill(rp, clearrects[i].MinX,
		    		 clearrects[i].MinY,
				 clearrects[i].MaxX,
				 clearrects[i].MaxY);
		}
		
		SetAfPt(rp, 0, 0);
		
	    } /* if (clear) */
	    	    
	    if (flags & AUTOKNOB)
	    {
		int hit = ((flags & KNOBHIT) != 0);

		if (flags & PROPNEWLOOK)
		{
	            if (IS_BORDER_GADGET(gadget))
		    {
			if (flags & PROPBORDERLESS)
			{
		    	    SetAPen(rp,dri->dri_Pens[SHINEPEN]);

			    /* Top edge */
			    RectFill(rp,knob->Left,
					knob->Top,
					knob->Left + knob->Width - 2,
					knob->Top);

			    /* Left edge */	    
			    RectFill(rp,knob->Left,
					knob->Top + 1,
					knob->Left,
					knob->Top + knob->Height - 2);

			    SetAPen(rp,dri->dri_Pens[SHADOWPEN]);

			    /* Right edge */
			    RectFill(rp,knob->Left + knob->Width - 1,
					knob->Top,
					knob->Left + knob->Width - 1,
					knob->Top + knob->Height - 1);

			    /* Bottom edge */	    
			    RectFill(rp,knob->Left,
					knob->Top + knob->Height - 1,
					knob->Left + knob->Width - 2,
					knob->Top + knob->Height - 1);

		    	    knob->Left++;
			    knob->Top++;
			    knob->Width -= 2;
			    knob->Height -= 2;
					
			} /* PROPBORDERLESS */
			else
			{
		            SetAPen(rp,dri->dri_Pens[SHADOWPEN]);

		    	    if (flags & FREEHORIZ)
			    {
				/* black line at the left and at the right */

				RectFill(rp,knob->Left,
			    		    knob->Top,
					    knob->Left,
					    knob->Top + knob->Height - 1);

				RectFill(rp,knob->Left + knob->Width - 1,
			    		    knob->Top,
					    knob->Left + knob->Width - 1,
					    knob->Top + knob->Height - 1);

				knob->Left++,
				knob->Width -= 2;
			    }
			    
			    if (flags & FREEVERT)
			    {
				/* black line at the top and at the bottom */

				RectFill(rp,knob->Left,
			    		    knob->Top,
					    knob->Left + knob->Width - 1,
					    knob->Top);

				RectFill(rp,knob->Left,
			    		    knob->Top + knob->Height - 1,
					    knob->Left + knob->Width - 1,
					    knob->Top + knob->Height - 1);

				knob->Top++;
				knob->Height -= 2;
			    }

			    
			} /* not PROPBORDERLESS */

			SetAPen(rp, dri->dri_Pens[(window->Flags & WFLG_WINDOWACTIVE) ? FILLPEN : BACKGROUNDPEN]);

			/* interior */
			RectFill(rp,knob->Left,
		    		    knob->Top,
				    knob->Left + knob->Width - 1,
				    knob->Top + knob->Height - 1);

		    } /* gadget inside window border */
		    else
		    {
			if (flags & PROPBORDERLESS)
			{
		    	    SetAPen(rp,dri->dri_Pens[SHADOWPEN]);

			    /* paint black right and bottom edges */

			    RectFill(rp,knob->Left + knob->Width - 1,
					knob->Top,
					knob->Left + knob->Width - 1,
					knob->Top + knob->Height - 1);

			    RectFill(rp,knob->Left,
					knob->Top + knob->Height - 1,
					knob->Left + knob->Width - 2,
					knob->Top + knob->Height - 1);

		    	    knob->Width--;
			    knob->Height--;
			    
			} /* propborderless */
			else
			{
		            SetAPen(rp,dri->dri_Pens[SHADOWPEN]);

		    	    if (flags & FREEHORIZ)
			    {
				/* black line at the left and at the right */

				RectFill(rp,knob->Left,
			    		    knob->Top,
					    knob->Left,
					    knob->Top + knob->Height - 1);

				RectFill(rp,knob->Left + knob->Width - 1,
			    		    knob->Top,
					    knob->Left + knob->Width - 1,
					    knob->Top + knob->Height - 1);

				knob->Left++,
				knob->Width -= 2;
			    }
			    if (flags & FREEVERT)
			    {
				/* black line at the top and at the bottom */

				RectFill(rp,knob->Left,
			    		    knob->Top,
					    knob->Left + knob->Width - 1,
					    knob->Top);

				RectFill(rp,knob->Left,
			    		    knob->Top + knob->Height - 1,
					    knob->Left + knob->Width - 1,
					    knob->Top + knob->Height - 1);

				knob->Top++;
				knob->Height -= 2;
			    }
			    
			} /* not propborderless */

			SetAPen(rp, dri->dri_Pens[SHINEPEN]);

			/* interior */
			RectFill(rp,knob->Left,
		    		    knob->Top,
				    knob->Left + knob->Width - 1,
				    knob->Top + knob->Height - 1);

		    } /* gadget not inside window border */

		} /* if (flags & PROPNEWLOOK) */
		else
		{
	    	    /* very old and ugly look */

		    SetAPen (rp, dri->dri_Pens[hit ? FILLPEN : SHADOWPEN]);

		    RectFill (rp
			, knob->Left
			, knob->Top
			, knob->Left + knob->Width - 1
			, knob->Top + knob->Height - 1
		    );

		} /* not PROPNEWLOOK */

	    } /* if (flags & AUTOKNOB) */
	    else
	    {
	        ((struct Image *)gadget->GadgetRender)->LeftEdge = 0;
		((struct Image *)gadget->GadgetRender)->TopEdge = 0;
		
	        DrawImageState(rp,
			       (struct Image *)gadget->GadgetRender,
			       knob->Left,
			       knob->Top,
			       IDS_NORMAL,
			       dri);
	    }
	    
	    if (gadget->Flags & GFLG_DISABLED)
	    {
	        struct BBox bbox;
		
		CalcBBox (window, gadget, &bbox);
 
		RenderDisabledPattern(rp, dri, bbox.Left,
	    				       bbox.Top,
					       bbox.Left + bbox.Width - 1,
					       bbox.Top + bbox.Height - 1,
					       IntuitionBase);	    
	    }
	    
	    ReleaseGIRPort(rp);
	    
	} /* if ((rp = ObtainGIRPort(&gi))) */
	
	FreeScreenDrawInfo(window->WScreen, dri);
	
    } /* if ((dri = GetScreenDrawInfo(window->WScreen))) */
        
    ReturnVoid("RefreshPropGadgetKnob");
    
} /* RefreshPropGadgetKnob */

