/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Routines for BOOL Gadgets
    Lang: english
*/

#include <proto/graphics.h>
#include <proto/intuition.h>
#include <intuition/cghooks.h>
#include "intuition_intern.h"
#include <graphics/gfxmacros.h>
#include "gadgets.h"

void RefreshBoolGadget (struct Gadget * gadget, struct Window * window,
	    struct IntuitionBase * IntuitionBase)
{
    struct GadgetInfo gi;
    struct RastPort *rp;
    struct BBox bbox;

    APTR  render;

#define RENDERGADGET(win,gad,rend)              \
	if (rend)                               \
	{					\
	    if (gad->Flags & GFLG_GADGIMAGE)    \
	    {					\
		DrawImage (rp                   \
		    , (struct Image *)rend      \
		    , bbox.Left 		\
		    , bbox.Top			\
		);				\
	    }					\
	    else				\
	    {					\
		DrawBorder (rp                  \
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

    SET_GI_RPORT(&gi, window, gadget);
    gi.gi_Layer = gi.gi_RastPort->Layer;
    
    rp = ObtainGIRPort(&gi);
    if (!rp) return;
    
    SetDrMd (rp, JAM1);

    EraseRect (rp
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
	    PrintIText (rp
		, gadget->GadgetText
		, bbox.Left
		, bbox.Top
	    );
	    break;

	case GFLG_LABELSTRING: {
	    STRPTR text = (STRPTR) gadget->GadgetText;
	    int len, labelwidth, labelheight;

	    len = strlen (text);

	    labelwidth = LabelWidth (rp, text, len, IntuitionBase);
	    labelheight = rp->Font->tf_YSize;

	    SetAPen (rp, 1);
	    SetDrMd (rp, JAM1);

	    Move (rp
		, bbox.Left + bbox.Width/2 - labelwidth/2
		, bbox.Top + bbox.Height/2 - labelheight/2
		    + rp->Font->tf_Baseline
	    );
	    RenderLabel (rp, text, len, IntuitionBase);

	    break; }

	case GFLG_LABELIMAGE:
	    DrawImage (rp
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
	    SetDrMd (rp, COMPLEMENT);

	    RectFill (rp
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
	    SetDrMd (rp, COMPLEMENT);

#define BOXWIDTH 5
	    RectFill (rp
		, bbox.Left
		, bbox.Top
		, bbox.Left + bbox.Width - 1
		, bbox.Top + bbox.Height - 1
	    );

	    if (bbox.Width > 2*BOXWIDTH && bbox.Height > 2*BOXWIDTH)
	    {
		RectFill (rp
		    , bbox.Left + BOXWIDTH
		    , bbox.Top + BOXWIDTH
		    , bbox.Left + bbox.Width - BOXWIDTH - 1
		    , bbox.Top + bbox.Height - BOXWIDTH - 1
		);
	    }
	}

	break;
    } /* Highlight after contents have been drawn */

    if ( gadget->Flags & GFLG_DISABLED )
    {
	UWORD pattern[] = { 0x8888, 0x2222 };

	SetDrMd( rp, JAM1 );
	SetAPen( rp, 1 );
	SetAfPt( rp, pattern, 1);

	/* render disable pattern */
	RectFill(rp,
	    bbox.Left,
	    bbox.Top,
	    bbox.Left + bbox.Width - 1,
	    bbox.Top + bbox.Height - 1 );
    }

    ReleaseGIRPort(rp);
    
} /* RefreshBoolGadget */
