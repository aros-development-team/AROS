/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/graphics.h>
#include "intuition_intern.h"
#include <string.h>

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

    DeinitRastPort(&rp);

    return width;

    AROS_LIBFUNC_EXIT
} /* IntuiTextLength */
