/*
    Copyright � 1995-2013, The AROS Development Team. All rights reserved.
    Copyright � 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/graphics.h>

#include "intuition_intern.h"

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
            the system's font is used (and *not* the font of the currently
            active screen!).

    RESULT
        The width of the text in pixels.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct RastPort  rp;
    struct TextFont *newfont = NULL;
    LONG    	     width;

    DEBUG_INTUITEXTLENGTH(dprintf("IntuiTextLength(itext 0x%lx)\n",iText));

    SANITY_CHECKR(iText,0)

    InitRastPort(&rp);

    if (iText->ITextFont)
    {
        newfont = OpenFont(iText->ITextFont);

        if (newfont)
            SetFont(&rp, newfont);
    }

    width = TextLength(&rp, iText->IText, strlen(iText->IText));

    if (newfont)
        CloseFont(newfont);

    return width;

    AROS_LIBFUNC_EXIT
} /* IntuiTextLength */
