/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/29 13:57:38  digulla
    Commented
    Moved common code from driver to Intuition

    Revision 1.2  1996/08/29 07:50:49  digulla
    Fixed a small bug in PropGadgets. The jumpsize of the knob was too small.

    Revision 1.1  1996/08/28 17:55:36  digulla
    Proportional gadgets
    BOOPSI


    Desc:
    Lang: english
*/
#include <clib/graphics_protos.h>
#include "intuition_intern.h"

int CalcKnobSize (struct Gadget * propGadget, long * knobleft, long * knobtop,
	long * knobwidth, long * knobheight)
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
	*knobleft += 3;
	*knobtop += 3;
	*knobwidth -= 6;
	*knobheight -= 6;
	pi->LeftBorder = 3;
	pi->TopBorder  = 3;
    }

    pi->CWidth	   = *knobwidth;
    pi->CHeight    = *knobheight;

    if (*knobwidth < KNOBHMIN || *knobheight < KNOBVMIN)
	return FALSE;

    if (pi->Flags & FREEHORIZ)
    {
	*knobwidth = pi->CWidth * pi->HorizBody / MAXBODY;

	*knobleft = *knobleft + (pi->CWidth - *knobwidth)
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
	*knobheight = pi->CHeight * pi->VertBody / MAXBODY;

	*knobtop = *knobtop + (pi->CHeight - *knobheight)
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
}

/*****************************************************************************

    NAME */
	#include <clib/intuition_protos.h>

	__AROS_LH4(void, RefreshGList,

/*  SYNOPSIS */
	__AROS_LHA(struct Gadget    *, gadgets, A0),
	__AROS_LHA(struct Window    *, window, A1),
	__AROS_LHA(struct Requester *, requester, A2),
	__AROS_LHA(long              , numGad, D0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 72, Intuition)

/*  FUNCTION
	Refresh (draw anew) the specified number of gadgets starting
	at the specified gadget.

    INPUTS
	gadgets - This is the first gadget which will be refreshed.
	window - The window which contains the gadget
	requester - If the gadget has GTYP_REQGADGET set, this must be
		a pointer to a Requester; otherwise the value is
		ignored.
	numGad - How many gadgets should be refreshed. The value
		may range from 0 to MAXLONG. If there are less gadgets
		in the list than numGad, only the gadgets in the
		list will be refreshed.

    RESULT
	None.

    NOTES
	This function *must not* be called inside a
	BeginRefresh()/EndRefresh() pair.

    EXAMPLE
	// Refresh one gadget
	RefreshGList (&gadget, win, NULL, 1);

	// Refresh all gadgets in the window
	RefreshGList (win->FirstGadget, win, NULL, -1L);

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    APTR render;
    WORD left, top, width, height;
    UBYTE DrawMode;
    ULONG apen;

#define ADDREL(flag,field)  ((gadgets->Flags & (flag)) ? window->field : 0)
#define RENDERGADGET(win,gad,rend)              \
	if (rend)                               \
	{					\
	    if (gad->Flags & GFLG_GADGIMAGE)    \
	    {					\
		DrawImage (win->RPort           \
		    , (struct Image *)rend      \
		    , left			\
		    , top			\
		);				\
	    }					\
	    else				\
	    {					\
		DrawBorder (win->RPort          \
		    , (struct Border *)rend     \
		    , left			\
		    , top			\
		);				\
	    }					\
	}
#define GETRENDER(gad)  (gad->Flags & GFLG_SELECTED) ? \
			    gad->SelectRender : gad->GadgetRender;

    apen = GetAPen (window->RPort);
    DrawMode = GetDrMd (window->RPort);

    for ( ; gadgets && numGad; gadgets=gadgets->NextGadget, numGad --)
    {
	left   = ADDREL(GFLG_RELRIGHT,Width)   + gadgets->LeftEdge;
	top    = ADDREL(GFLG_RELBOTTOM,Height) + gadgets->TopEdge;
	width  = ADDREL(GFLG_RELWIDTH,Width)   + gadgets->Width;
	height = ADDREL(GFLG_RELHEIGHT,Height) + gadgets->Height;

	if (width <= 0 || height <= 0)
	    continue;

	SetDrMd (window->RPort, JAM1);
	SetAPen (window->RPort, 0);

	RectFill (window->RPort
	    , left
	    , top
	    , left + width - 1
	    , top + height - 1
	);

	switch (gadgets->GadgetType & GTYP_GTYPEMASK)
	{
	case GTYP_BOOLGADGET:
	    if (gadgets->GadgetText)
	    {
		switch (gadgets->Flags & GFLG_LABELMASK)
		{
		case GFLG_LABELITEXT:
		    PrintIText (window->RPort
			, gadgets->GadgetText
			, left
			, top
		    );
		    break;

		case GFLG_LABELSTRING: {
		    STRPTR text = (STRPTR) gadgets->GadgetText;
		    int len, labelwidth, labelheight;

		    len = strlen (text);

		    labelwidth = TextLength (window->RPort, text, len);
		    labelheight = window->RPort->Font->tf_YSize;

		    SetAPen (window->RPort, 1);
		    SetDrMd (window->RPort, JAM1);

		    Move (window->RPort
			, left + width/2 - labelwidth/2
			, top + height/2 - labelheight/2
			    + window->RPort->Font->tf_Baseline
		    );
		    Text (window->RPort, text, len);

		    break; }

		case GFLG_LABELIMAGE:
		    DrawImage (window->RPort
			, (struct Image *)gadgets->GadgetText
			, left
			, top
		    );
		    break;
		}
	    }
	    switch (gadgets->Flags & GFLG_GADGHIGHBITS)
	    {
	    case GFLG_GADGHCOMP: {
		render = gadgets->GadgetRender;
		RENDERGADGET(window,gadgets,render);

		if (gadgets->Flags & GFLG_SELECTED)
		{
		    SetDrMd (window->RPort, COMPLEMENT);

		    RectFill (window->RPort
			, left
			, top
			, left + width - 1
			, top + height - 1
		    );
		}

		break; }

	    case GFLG_GADGHIMAGE:
		render = GETRENDER(gadgets);
		RENDERGADGET(window,gadgets,render);
		break;

	    case GFLG_GADGHNONE:
		render = gadgets->GadgetRender;
		RENDERGADGET(window,gadgets,render);
		break;

	    case GFLG_GADGHBOX:
		render = gadgets->GadgetRender;
		RENDERGADGET(window,gadgets,render);

		if (gadgets->Flags & GFLG_SELECTED)
		{
		    SetDrMd (window->RPort, COMPLEMENT);

	#define BOXWIDTH 5
		    RectFill (window->RPort
			, left
			, top
			, left + width - 1
			, top + height - 1
		    );

		    if (width > 2*BOXWIDTH && height > 2*BOXWIDTH)
		    {
			RectFill (window->RPort
			    , left + BOXWIDTH
			    , top + BOXWIDTH
			    , left + width - BOXWIDTH - 1
			    , top + height - BOXWIDTH - 1
			);
		    }
		}

		break;

	    } /* switch GadgetHighlightMethod */

	    break; /* BOOLGADGET */

	case GTYP_GADGET0002:
	    break;

	case GTYP_PROPGADGET: {
	    long knobleft, knobtop;
	    long knobwidth, knobheight;
	    struct PropInfo * pi;

	    SetDrMd (window->RPort, JAM1);

	    pi = (struct PropInfo *)gadgets->SpecialInfo;

	    if (!pi)
		break;

	    knobleft = left;
	    knobtop = top;
	    knobwidth = width;
	    knobheight = height;

	    if (!CalcKnobSize (gadgets, &knobleft, &knobtop, &knobwidth, &knobheight))
		break;

	    if (!(pi->Flags & PROPBORDERLESS) )
	    {
		if (pi->Flags & PROPNEWLOOK)
		{
		    if (width <= 6 || height <= 6)
		    {
			SetAPen (window->RPort, 2);

			RectFill (window->RPort
			    , left
			    , top
			    , left + width - 1
			    , top + height - 1
			);

			break;
		    }
		    else
		    {
			SetAPen (window->RPort, 2);

			/* right */
			RectFill (window->RPort
			    , left + width - 2
			    , top
			    , left + width - 1
			    , top + height - 1
			);

			/* bottom */
			RectFill (window->RPort
			    , left
			    , top + height - 2
			    , left + width - 3
			    , top + height - 1
			);

			SetAPen (window->RPort, 1);

			/* top */
			RectFill (window->RPort
			    , left
			    , top
			    , left + width - 2
			    , top + 1
			);

			/* left */
			RectFill (window->RPort
			    , left
			    , top
			    , left + 1
			    , top + height - 2
			);

			WritePixel (window->RPort, left + width - 1, top);
			WritePixel (window->RPort, left, top + height - 1);
		    }
		}
		else
		{
		    SetAPen (window->RPort, 2);

		    if (width <= 6 || height <= 6)
		    {
			RectFill (window->RPort
			    , left
			    , top
			    , left + width - 1
			    , top + height - 1
			);

			break;
		    }
		    else
		    {
			/* right */
			RectFill (window->RPort
			    , left + width - 2
			    , top
			    , left + width - 1
			    , top + height - 1
			);

			/* bottom */
			RectFill (window->RPort
			    , left
			    , top + height - 2
			    , left + width - 1
			    , top + height - 1
			);

			/* top */
			RectFill (window->RPort
			    , left
			    , top
			    , left + width - 3
			    , top + 1
			);

			/* left */
			RectFill (window->RPort
			    , left
			    , top
			    , left + 1
			    , top + height - 3
			);
		    }
		}
	    }

	    if (pi->Flags & AUTOKNOB)
	    {
		int hit = ((pi->Flags & KNOBHIT) != 0);

		if (pi->Flags & PROPNEWLOOK)
		{
		    SetAPen (window->RPort, hit ? 2 : 1);

		    /* Draw right border */
		    RectFill (window->RPort
			, knobleft + knobwidth - 2
			, knobtop
			, knobleft + knobwidth - 1
			, knobtop + knobheight - 1
		    );

		    /* Draw bottom border */
		    RectFill (window->RPort
			, knobleft
			, knobtop + knobheight - 2
			, knobleft + knobwidth - 3
			, knobtop + knobheight - 1
		    );

		    SetAPen (window->RPort, hit ? 1 : 2);

		    /* Draw top border */
		    RectFill (window->RPort
			, knobleft
			, knobtop
			, knobleft + knobwidth - 2
			, knobtop + 1
		    );

		    /* Draw left border */
		    RectFill (window->RPort
			, knobleft
			, knobtop + 2
			, knobleft + 1
			, knobtop + knobheight - 2
		    );

		    /* Fill edges */
		    WritePixel (window->RPort, knobleft + knobwidth - 1, knobtop);
		    WritePixel (window->RPort, knobleft, knobtop + knobheight - 1);
		}
		else
		{
		    SetAPen (window->RPort, 2);

		    RectFill (window->RPort
			, knobleft
			, knobtop
			, knobleft + knobwidth - 1
			, knobtop + knobheight - 1
		    );
		}
	    }

	    break; }

	case GTYP_STRGADGET:
	    break;

	case GTYP_CUSTOMGADGET:
	    break;

	} /* switch GadgetType */
    }

    SetDrMd (window->RPort, DrawMode);
    SetAPen (window->RPort, apen);

    __AROS_FUNC_EXIT
} /* RefreshGList */
