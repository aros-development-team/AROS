/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Code for PROP Gadgets
    Lang: english
*/
#include <proto/graphics.h>
#include <proto/intuition.h>
#include "intuition_intern.h"
#include "propgadgets.h"
#include "gadgets.h"

#define DEBUG 0
#include <aros/debug.h>


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

    if (!CalcKnobSize (gadget, &knob))
	return;

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
	    if (dx + pi->HPotRes < MAXPOT)
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
	    if (dy + pi->VPotRes < MAXPOT)
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
	if (pi->Flags & FREEHORIZ
	    && pi->CWidth != knob.Width)
	{
	    dx = (dx * MAXPOT) /(pi->CWidth - knob.Width);

	    if (dx < 0)
	    {
		dx = -dx;

		if (dx > pi->HorizPot)
		    dx = 0;
		else
		    dx = pi->HorizPot - dx;
	    }
	    else
	    {
	    if (dx + pi->HorizPot > MAXPOT)
		dx = MAXPOT;
	    else
		dx = pi->HorizPot + dx;
	    }
	} /* FREEHORIZ */

	if (pi->Flags & FREEVERT
	    && pi->CHeight != knob.Height)
	{
	    dy = (dy * MAXPOT) / (pi->CHeight - knob.Height);

	    if (dy < 0)
	    {
		dy = -dy;

		if (dy > pi->VertPot)
		    dy = 0;
		else
		    dy = pi->VertPot - dy;
	    }
	    else
	    {
		if (dy + pi->VertPot > MAXPOT)
		    dy = MAXPOT;
		else
		    dy = pi->VertPot + dy;
	    }
	} /* FREEVERT */
    } /* Has PropInfo and Mouse is over knob */

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
	
    return;
}
    
int CalcKnobSize (struct Gadget * propGadget, struct BBox * knobbox)
{
    struct PropInfo * pi;

    pi = (struct PropInfo *)propGadget->SpecialInfo;

    if (pi->Flags & PROPBORDERLESS)
    {
	pi->LeftBorder = 0;
	pi->TopBorder  = 0;
    }
    else
    {
	knobbox->Left += 3;
	knobbox->Top += 3;
	knobbox->Width -= 6;
	knobbox->Height -= 6;
	pi->LeftBorder = 3;
	pi->TopBorder  = 3;
    }

    pi->CWidth	   = knobbox->Width;
    pi->CHeight    = knobbox->Height;

    if (knobbox->Width < KNOBHMIN || knobbox->Height < KNOBVMIN)
	return FALSE;

    if (pi->Flags & FREEHORIZ)
    {
	knobbox->Width = pi->CWidth * pi->HorizBody / MAXBODY;

	knobbox->Left = knobbox->Left + (pi->CWidth - knobbox->Width)
		* pi->HorizPot / MAXPOT;

	if (pi->HorizBody)
	{
	    if (pi->HorizBody < MAXBODY/2)
		pi->HPotRes = MAXPOT / ((MAXBODY / pi->HorizBody) - 1);
	    else
		pi->HPotRes = MAXPOT;
	}
	else
	    pi->HPotRes = 1;
    }

    if (pi->Flags & FREEVERT)
    {
	knobbox->Height = pi->CHeight * pi->VertBody / MAXBODY;

	knobbox->Top = knobbox->Top + (pi->CHeight - knobbox->Height)
		* pi->VertPot / MAXPOT;

	if (pi->VertBody)
	{
	    if (pi->VertBody < MAXBODY/2)
		pi->VPotRes = MAXPOT / ((MAXBODY / pi->VertBody) - 1);
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
    UBYTE DrawMode;
    ULONG apen;
    struct PropInfo * pi;
    struct BBox bbox, kbox;
    D(bug("RefreshPropGadget(gad=%p, win=%s)\n", gadget, window->Title));
    
    CalcBBox (window, gadget, &bbox);

    if (bbox.Width <= 0 || bbox.Height <= 0)
	return;


    apen = GetAPen (window->RPort);
    DrawMode = GetDrMd (window->RPort);

    SetDrMd (window->RPort, JAM1);

    EraseRect (window->RPort
	, bbox.Left
	, bbox.Top
	, bbox.Left + bbox.Width - 1
	, bbox.Top + bbox.Height - 1
    );

    pi = (struct PropInfo *)gadget->SpecialInfo;

    if (!pi)
	return;

    kbox.Left	= bbox.Left;
    kbox.Top	= bbox.Top;
    kbox.Width	= bbox.Width;
    kbox.Height = bbox.Height;

    if (!CalcKnobSize (gadget, &kbox))
	return;

    if (!(pi->Flags & PROPBORDERLESS) )
    {
	if (pi->Flags & PROPNEWLOOK)
	{
	    if (bbox.Width <= 6 || bbox.Height <= 6)
	    {
		SetAPen (window->RPort, 2);

		RectFill (window->RPort
		    , bbox.Left
		    , bbox.Top
		    , bbox.Left + bbox.Width - 1
		    , bbox.Top + bbox.Height - 1
		);

		return;
	    }
	    else
	    {
		SetAPen (window->RPort, 2);

		/* right */
		RectFill (window->RPort
		    , bbox.Left + bbox.Width - 2
		    , bbox.Top
		    , bbox.Left + bbox.Width - 1
		    , bbox.Top + bbox.Height - 1
		);

		/* bottom */
		RectFill (window->RPort
		    , bbox.Left
		    , bbox.Top + bbox.Height - 2
		    , bbox.Left + bbox.Width - 3
		    , bbox.Top + bbox.Height - 1
		);

		SetAPen (window->RPort, 1);

		/* top */
		RectFill (window->RPort
		    , bbox.Left
		    , bbox.Top
		    , bbox.Left + bbox.Width - 2
		    , bbox.Top + 1
		);

		/* left */
		RectFill (window->RPort
		    , bbox.Left
		    , bbox.Top
		    , bbox.Left + 1
		    , bbox.Top + bbox.Height - 2
		);

		WritePixel (window->RPort, bbox.Left + bbox.Width - 1, bbox.Top);
		WritePixel (window->RPort, bbox.Left, bbox.Top + bbox.Height - 1);
	    }
	}
	else
	{
	    SetAPen (window->RPort, 2);

	    if (bbox.Width <= 6 || bbox.Height <= 6)
	    {
		RectFill (window->RPort
		    , bbox.Left
		    , bbox.Top
		    , bbox.Left + bbox.Width - 1
		    , bbox.Top + bbox.Height - 1
		);

		return;
	    }
	    else
	    {
		/* right */
		RectFill (window->RPort
		    , bbox.Left + bbox.Width - 2
		    , bbox.Top
		    , bbox.Left + bbox.Width - 1
		    , bbox.Top + bbox.Height - 1
		);

		/* bottom */
		RectFill (window->RPort
		    , bbox.Left
		    , bbox.Top + bbox.Height - 2
		    , bbox.Left + bbox.Width - 1
		    , bbox.Top + bbox.Height - 1
		);

		/* top */
		RectFill (window->RPort
		    , bbox.Left
		    , bbox.Top
		    , bbox.Left + bbox.Width - 3
		    , bbox.Top + 1
		);

		/* left */
		RectFill (window->RPort
		    , bbox.Left
		    , bbox.Top
		    , bbox.Left + 1
		    , bbox.Top + bbox.Height - 3
		);
	    }
	}
    }

    RefreshPropGadgetKnob (pi->Flags, NULL, &kbox, window, IntuitionBase);

    SetDrMd (window->RPort, DrawMode);
    SetAPen (window->RPort, apen);
    
    ReturnVoid("RefreshPropGadget");
} /* RefreshPropGadget */


void RefreshPropGadgetKnob (UWORD flags, struct BBox * clear,
	struct BBox * knob, struct Window * window,
	struct IntuitionBase * IntuitionBase)
{
    UBYTE DrawMode;
    ULONG apen;
    
    D(bug("RefresPropGadgetKnob(flags=%d, clear=%p, knob = %p, win=%s)\n",
    	flags, clear, knob, window->Title));

    apen = GetAPen (window->RPort);
    DrawMode = GetDrMd (window->RPort);

    SetDrMd (window->RPort, JAM1);

    if (clear && clear->Width > 0 && clear->Height > 0)
    {
	EraseRect (window->RPort
	    , clear->Left
	    , clear->Top
	    , clear->Left + clear->Width - 1
	    , clear->Top + clear->Height - 1
	);
    }

    if (flags & AUTOKNOB)
    {
	int hit = ((flags & KNOBHIT) != 0);

	if (flags & PROPNEWLOOK)
	{
	    SetAPen (window->RPort, hit ? 2 : 1);

	    /* Draw right border */
	    RectFill (window->RPort
		, knob->Left + knob->Width - 2
		, knob->Top
		, knob->Left + knob->Width - 1
		, knob->Top + knob->Height - 1
	    );

	    /* Draw bottom border */
	    RectFill (window->RPort
		, knob->Left
		, knob->Top + knob->Height - 2
		, knob->Left + knob->Width - 3
		, knob->Top + knob->Height - 1
	    );

	    SetAPen (window->RPort, hit ? 1 : 2);

	    /* Draw top border */
	    RectFill (window->RPort
		, knob->Left
		, knob->Top
		, knob->Left + knob->Width - 2
		, knob->Top + 1
	    );

	    /* Draw left border */
	    RectFill (window->RPort
		, knob->Left
		, knob->Top + 2
		, knob->Left + 1
		, knob->Top + knob->Height - 2
	    );

	    /* Fill edges */
	    WritePixel (window->RPort, knob->Left + knob->Width - 1, knob->Top);

	}
	else
	{
	    SetAPen (window->RPort, 2);

	    RectFill (window->RPort
		, knob->Left
		, knob->Top
		, knob->Left + knob->Width - 1
		, knob->Top + knob->Height - 1
	    );
	}
    }

    SetDrMd (window->RPort, DrawMode);
    SetAPen (window->RPort, apen);
    
    ReturnVoid("RefreshPropGadgetKnob");
    
} /* RefreshPropGadgetKnob */

