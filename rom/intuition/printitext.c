/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <string.h>
#include <proto/graphics.h>
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <graphics/rpattr.h>
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

    IPTR   	     apen;
    IPTR   	     bpen;
    IPTR   	     drmd;
    IPTR   	     penmode;
    UBYTE   	     style;
    struct TextFont *font;
    struct TextFont *newfont = NULL;

    EXTENDWORD(leftOffset);
    EXTENDWORD(topOffset);

    DEBUG_PRINTITEXT(dprintf("PrintIText: rp %p text %p Left %ld Top %ld\n",
                 rp, iText, leftOffset, topOffset));

    /* Store important variables of the RastPort */
#ifdef __MORPHOS__
    GetRPAttrs(rp,RPTAG_PenMode,(IPTR)&penmode,RPTAG_APen,(IPTR)&apen,
               RPTAG_BPen,(IPTR)&bpen,RPTAG_DrMd,(IPTR)&drmd,TAG_DONE);
#else
    GetRPAttrs(rp,RPTAG_APen,(IPTR)&apen,
               RPTAG_BPen,(IPTR)&bpen,RPTAG_DrMd,(IPTR)&drmd,TAG_DONE);
#endif

    font  = rp->Font;
    style = rp->AlgoStyle;

    /* For all borders... */
    for ( ; iText; iText = iText->NextText)
    {
        /* Change RastPort to the colors/mode specified */
        SetABPenDrMd (rp, iText->FrontPen, iText->BackPen, iText->DrawMode);

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
#ifdef __MORPHOS__
    SetRPAttrs(rp,RPTAG_APen,apen,RPTAG_BPen,bpen,RPTAG_DrMd,drmd,RPTAG_PenMode,penmode,TAG_DONE);
#else
    SetRPAttrs(rp,RPTAG_APen,apen,RPTAG_BPen,bpen,RPTAG_DrMd,drmd,TAG_DONE);
#endif
    SetFont      (rp, font);
    SetSoftStyle (rp, style, AskSoftStyle(rp));

    AROS_LIBFUNC_EXIT
} /* PrintIText */
