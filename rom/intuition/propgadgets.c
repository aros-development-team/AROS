/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Code for PROP Gadgets
    Lang: english
*/
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#include "intuition_intern.h"
#include "propgadgets.h"
#include "gadgets.h"

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
} /* RefreshPropGadget */


void RefreshPropGadgetKnob (UWORD flags, struct BBox * clear,
	struct BBox * knob, struct Window * window,
	struct IntuitionBase * IntuitionBase)
{
    UBYTE DrawMode;
    ULONG apen;

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
} /* RefreshPropGadgetKnob */

