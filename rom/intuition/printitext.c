/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.12  2000/08/12 19:21:28  stegerg
    if there is a IntuiText->ITextFont must SetSoftStyle(rp,
    IntuiText->ITextFont->ta_Style)

    Revision 1.11  1998/10/20 16:46:01  hkiel
    Amiga Research OS

    Revision 1.10  1998/08/16 18:48:34  nlorentz
    Bugfix: Now renders text so that TopEdge is top of character cell

    Revision 1.9  1998/02/12 16:19:45  turrican
    Fix uninitialized variable warnings

    Revision 1.8  1997/01/27 00:36:42  ldp
    Polish

    Revision 1.7  1996/12/10 14:00:07  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.6  1996/11/08 11:28:04  aros
    All OS function use now Amiga types

    Moved intuition-driver protos to intuition_intern.h

    Revision 1.5  1996/10/24 15:51:23  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/10/21 17:07:08  aros
    Restored wrong font

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
