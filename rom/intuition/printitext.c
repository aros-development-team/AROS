/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/29 15:13:32  digulla
    Wrote code

    Revision 1.2  1996/08/29 13:57:37  digulla
    Commented
    Moved common code from driver to Intuition

    Revision 1.1  1996/08/23 17:28:18  digulla
    Several new functions; some still empty.


    Desc:
    Lang: english
*/
#include "intuition_intern.h"
#include <string.h>
#include <clib/graphics_protos.h>

/*****************************************************************************

    NAME */
	#include <graphics/rastport.h>
	#include <intuition/intuition.h>
	#include <clib/intuition_protos.h>

	__AROS_LH4(void, PrintIText,

/*  SYNOPSIS */
	__AROS_LHA(struct RastPort  *, rp, A0),
	__AROS_LHA(struct IntuiText *, iText, A1),
	__AROS_LHA(long              , leftOffset, D0),
	__AROS_LHA(long              , topOffset, D1),

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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    ULONG  apen;
    ULONG  bpen;
    ULONG  drmd;
    struct TextFont * font;
    struct TextFont * newfont;

    /* Store important variables of the RastPort */
    apen = GetAPen (rp);
    bpen = GetBPen (rp);
    drmd = GetDrMd (rp);

    font = rp->Font;

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
	}

	/* Move to initial position */
	Move (rp
	    , iText->LeftEdge + leftOffset
	    , iText->TopEdge + topOffset
	);
	Text (rp, iText->IText, strlen (iText->IText));

	if (iText->ITextFont)
	{
	    if (newfont)
	    {
		SetFont (rp, newfont);
		CloseFont (newfont);
	    }
	}
    }

    /* Restore RastPort */
    SetAPen (rp, apen);
    SetBPen (rp, bpen);
    SetDrMd (rp, drmd);
    SetFont (rp, font);

    __AROS_FUNC_EXIT
} /* PrintIText */
