/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
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
	pi->HPotRes = pi->CWidth * pi->HorizBody / MAXBODY;

	*knobleft = *knobleft + (pi->CWidth - pi->HPotRes)
		* pi->HorizPot / MAXPOT;
	*knobwidth = pi->HPotRes;
    }

    if (pi->Flags & FREEVERT)
    {
	pi->VPotRes = pi->CHeight * pi->VertBody / MAXBODY;

	*knobtop = *knobtop + (pi->CHeight - pi->VPotRes)
		* pi->VertPot / MAXPOT;
	*knobheight = pi->VPotRes;
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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

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

	switch (gadgets->GadgetType & GTYP_GTYPEMASK)
	{
	case GTYP_BOOLGADGET:
	    SetDrMd (window->RPort, JAM1);
	    SetAPen (window->RPort, 0);

	    RectFill (window->RPort
		, left
		, top
		, left + width - 1
		, top + height - 1
	    );

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
		SetAPen (window->RPort, 2);

		RectFill (window->RPort
		    , left
		    , top
		    , left + width - 1
		    , top + height - 1
		);

		SetAPen (window->RPort, 0);

		if (width <= 6 || height <= 6)
		    break;

		RectFill (window->RPort
		    , left + 2
		    , top + 2
		    , left + width - 3
		    , top + height - 3
		);
	    }

	    if (pi->Flags & AUTOKNOB)
	    {
		int hit = ((pi->Flags & KNOBHIT) != 0);

		if (pi->Flags & PROPNEWLOOK)
		{
		    SetAPen (window->RPort, hit ? 2 : 1);

		    /* Draw right border */
		    RectFill (window->RPort
			, knobleft + knobwidth - 3
			, knobtop
			, knobleft + knobwidth - 1
			, knobtop + knobheight - 1
		    );

		    /* Draw bottom border */
		    RectFill (window->RPort
			, knobleft
			, knobtop + knobheight - 3
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
