/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "intuition_intern.h"
#include <string.h>
#include <proto/graphics.h>

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <intuition/intuition.h>
#include <proto/intuition.h>

	AROS_LH4(void, PrintIText,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort  *, rp, A0),
	AROS_LHA(struct IntuiText *, iText, A1),
	AROS_LHA(LONG              , leftOffset, D0),
	AROS_LHA(LONG              , topOffset, D1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 36, Intuition)

/*  FUNCTION
	Render an IntuiText in the specified RastPort with the
	specified offset.

    INPUTS
	rp - Draw into this RastPort
	iText - Render this text
	leftOffset, topOffset - Starting-Point. All coordinates in the
		IntuiText structures are relative to this point.

    RESULT
	None.

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
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    ULONG  apen;
    ULONG  bpen;
    ULONG  drmd;
    UBYTE  style;
    struct TextFont * font;
    struct TextFont * newfont = NULL;

    /* Store important variables of the RastPort */
    apen  = GetAPen (rp);
    bpen  = GetBPen (rp);
    drmd  = GetDrMd (rp);
    font  = rp->Font;
    style = rp->AlgoStyle; 

    /* For all borders... */
    for ( ; iText; iText=iText->NextText)
    {
	/* Change RastPort to the colors/mode specified */
	SetAPen (rp, iText->FrontPen);
	SetBPen (rp, iText->BackPen);
	SetDrMd (rp, iText->DrawMode);

	if (iText->ITextFont)
	{
	    newfont = OpenFont (iText->ITextFont);

	    if (newfont)
		SetFont (rp, newfont);
		
	    SetSoftStyle(rp, iText->ITextFont->ta_Style, AskSoftStyle(rp));
	}

	/* Move to initial position */
	Move (rp
	    , iText->LeftEdge + leftOffset
	    , iText->TopEdge + topOffset + rp->Font->tf_Baseline
	);
	Text (rp, iText->IText, strlen (iText->IText));

	if (iText->ITextFont)
	{
	    if (newfont)
	    {
		SetFont (rp, font);
		CloseFont (newfont);
	    }
	}
    }

    /* Restore RastPort */
    SetAPen      (rp, apen);
    SetBPen      (rp, bpen);
    SetDrMd      (rp, drmd);
    SetFont      (rp, font);
    SetSoftStyle (rp, style, AskSoftStyle(rp));
    
    AROS_LIBFUNC_EXIT
} /* PrintIText */
