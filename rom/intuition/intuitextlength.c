/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.7  1999/02/18 23:18:01  hkiel
    Added return value

    Revision 1.6  1998/10/20 16:45:58  hkiel
    Amiga Research OS

    Revision 1.5  1997/01/27 00:36:40  ldp
    Polish

    Revision 1.4  1996/12/10 14:00:04  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.3  1996/10/31 13:51:18  aros
    Create and free the RastPort with the new functions

    Revision 1.2  1996/10/24 15:51:20  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/10/21 17:06:48  aros
    A couple of new functions


    Desc:
    Lang: english
*/
#include "intuition_intern.h"
#include <string.h>
#include <proto/graphics.h>

/*****************************************************************************

    NAME */
#include <intuition/intuition.h>
#include <proto/intuition.h>

	AROS_LH1(LONG, IntuiTextLength,

/*  SYNOPSIS */
	AROS_LHA(struct IntuiText *, iText, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 55, Intuition)

/*  FUNCTION
	Measure the length of the IntuiText passed to the function. Further
	IntuiTexts in iText->NextText are ignored. The length is measured in
	pixels.

    INPUTS
	iText - The size of this text. If iText->ITextFont contains NULL,
		the systems font is used (and *not* the font of the currently
		active screen !).

    RESULT
	The width of the text in pixels.

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
    struct RastPort * rp;
    struct TextFont * newfont;
    LONG width=0;

    rp = CreateRastPort ();

    if (rp)
    {
	if (iText->ITextFont)
	{
	    newfont = OpenFont (iText->ITextFont);

	    if (newfont)
		SetFont (rp, newfont);
	}

	width = TextLength (rp, iText->IText, strlen (iText->IText));

	if (iText->ITextFont)
	{
	    if (newfont)
		CloseFont (newfont);
	}

	FreeRastPort (rp);
    }

    return width;

    AROS_LIBFUNC_EXIT
} /* IntuiTextLength */
