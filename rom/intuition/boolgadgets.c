/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Routines for BOOL Gadgets
    Lang: english
*/

#include <proto/graphics.h>
#include <proto/intuition.h>
#include "intuition_intern.h"
#include "gadgets.h"

void RefreshBoolGadget (struct Gadget * gadget, struct Window * window,
	    struct IntuitionBase * IntuitionBase)
{
    struct BBox bbox;

    APTR  render;
    UBYTE DrawMode;
    ULONG apen;

#define RENDERGADGET(win,gad,rend)              \
	if (rend)                               \
	{					\
	    if (gad->Flags & GFLG_GADGIMAGE)    \
	    {					\
		DrawImage (win->RPort           \
		    , (struct Image *)rend      \
		    , bbox.Left 		\
		    , bbox.Top			\
		);				\
	    }					\
	    else				\
	    {					\
		DrawBorder (win->RPort          \
		    , (struct Border *)rend     \
		    , bbox.Left 		\
		    , bbox.Top			\
		);				\
	    }					\
	}
#define GETRENDER(gad)  (gad->Flags & GFLG_SELECTED) ? \
			    gad->SelectRender : gad->GadgetRender;


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

    switch (gadget->Flags & GFLG_GADGHIGHBITS)
    {
    case GFLG_GADGHIMAGE:
	render = GETRENDER(gadget);
	RENDERGADGET(window,gadget,render);
	break;

    case GFLG_GADGHNONE:
	render = gadget->GadgetRender;
	RENDERGADGET(window,gadget,render);
	break;

    } /* switch GadgetHighlightMethod */

    if (gadget->GadgetText)
    {
	switch (gadget->Flags & GFLG_LABELMASK)
	{

	case GFLG_LABELITEXT:
	    PrintIText (window->RPort
		, gadget->GadgetText
		, bbox.Left
		, bbox.Top
	    );
	    break;

	case GFLG_LABELSTRING: {
	    STRPTR text = (STRPTR) gadget->GadgetText;
	    int len, labelwidth, labelheight;

	    len = strlen (text);

	    labelwidth = LabelWidth (window->RPort, text, len, IntuitionBase);
	    labelheight = window->RPort->Font->tf_YSize;

	    SetAPen (window->RPort, 1);
	    SetDrMd (window->RPort, JAM1);

	    Move (window->RPort
		, bbox.Left + bbox.Width/2 - labelwidth/2
		, bbox.Top + bbox.Height/2 - labelheight/2
		    + window->RPort->Font->tf_Baseline
	    );
	    RenderLabel (window->RPort, text, len, IntuitionBase);

	    break; }

	case GFLG_LABELIMAGE:
	    DrawImage (window->RPort
		, (struct Image *)gadget->GadgetText
		, bbox.Left
		, bbox.Top
	    );
	    break;
	}
    } /* GadgetText */

    switch (gadget->Flags & GFLG_GADGHIGHBITS)
    {
    case GFLG_GADGHCOMP:
	render = gadget->GadgetRender;
	RENDERGADGET(window,gadget,render);

	if (gadget->Flags & GFLG_SELECTED)
	{
	    SetDrMd (window->RPort, COMPLEMENT);

	    RectFill (window->RPort
		, bbox.Left
		, bbox.Top
		, bbox.Left + bbox.Width - 1
		, bbox.Top + bbox.Height - 1
	    );
	}

	break;

    case GFLG_GADGHBOX:
	render = gadget->GadgetRender;
	RENDERGADGET(window,gadget,render);

	if (gadget->Flags & GFLG_SELECTED)
	{
	    SetDrMd (window->RPort, COMPLEMENT);

#define BOXWIDTH 5
	    RectFill (window->RPort
		, bbox.Left
		, bbox.Top
		, bbox.Left + bbox.Width - 1
		, bbox.Top + bbox.Height - 1
	    );

	    if (bbox.Width > 2*BOXWIDTH && bbox.Height > 2*BOXWIDTH)
	    {
		RectFill (window->RPort
		    , bbox.Left + BOXWIDTH
		    , bbox.Top + BOXWIDTH
		    , bbox.Left + bbox.Width - BOXWIDTH - 1
		    , bbox.Top + bbox.Height - BOXWIDTH - 1
		);
	    }
	}

	break;
    } /* Highlight after contents have been drawn */

    SetDrMd (window->RPort, DrawMode);
    SetAPen (window->RPort, apen);
} /* RefreshBoolGadget */
