/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/graphics.h>
#include <proto/dos.h>
#include <proto/layers.h>
#include <graphics/rpattr.h>
#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <graphics/rastport.h>
#include <intuition/intuition.h>
#include <proto/intuition.h>

AROS_LH4(void, DrawBorder,

         /*  SYNOPSIS */
         AROS_LHA(struct RastPort *, rp, A0),
         AROS_LHA(struct Border   *, border, A1),
         AROS_LHA(LONG             , leftOffset, D0),
         AROS_LHA(LONG             , topOffset, D1),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 18, Intuition)

/*  FUNCTION
    Draws one or more borders in the specified RastPort. Rendering
    will start at the position which you get when you add the offsets
    leftOffset and topOffset to the LeftEdge and TopEdge specified
    in the Border structure. All coordinates are relative to that point.
 
    INPUTS
    rp - The RastPort to render into
    border - Information what and how to render
    leftOffset, topOffset - Initial starting position
 
    RESULT
    None.
 
    NOTES
 
    EXAMPLE
    // Draw a house with one stroke
    // The drawing starts at the lower left edge
    WORD XY[] =
    {
        10, -10,
        10,   0,
         0, -10,
        10, -10,
         5, -15,
         0, -10,
         0,   0,
        10,   0,
    };
    struct Border demo =
    {
        100, 100,   // Position
        1, 2,   // Pens
        JAM1,   // Drawmode
        8,      // Number of pairs in XY
        XY,     // Vector offsets
        NULL    // No next border
    };
 
    // Render the house with the bottom left edge at 150, 50
    DrawBorder (rp, &demo, 50, -50);
 
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
    ULONG  penmode;
    WORD  *ptr;
    WORD   x, y;
    WORD   xoff, yoff;
    int    t;

    EXTENDWORD(leftOffset);EXTENDWORD(topOffset);

    DEBUG_DRAWBORDER(dprintf("DrawBorder: rp %p border %p Left %ld Top %ld\n",
                             rp, border, leftOffset, topOffset));

    SANITY_CHECK(rp)
    SANITY_CHECK(border)

    if (rp->Layer) LockLayer(0,rp->Layer);

    /* Store important variables of the RastPort */
#ifdef __MORPHOS__
    GetRPAttrs(rp,RPTAG_PenMode,(ULONG)&penmode,RPTAG_APen,(ULONG)&apen,
               RPTAG_BPen,(ULONG)&bpen,RPTAG_DrMd,(ULONG)&drmd,TAG_DONE);
#else
    GetRPAttrs(rp,RPTAG_APen,(ULONG)&apen,
               RPTAG_BPen,(ULONG)&bpen,RPTAG_DrMd,(ULONG)&drmd,TAG_DONE);
#endif

    /* For all borders... */
    for ( ; border; border = border->NextBorder)
    {
        /* Change RastPort to the colors/mode specified */
        SetAPen (rp, border->FrontPen);
        SetBPen (rp, border->BackPen);
        SetDrMd (rp, border->DrawMode);

        /* Get base coords */

        x = border->LeftEdge + leftOffset;
        y = border->TopEdge + topOffset;

        /* Start of vector offsets */
        ptr = border->XY;

        for (t = 0; t < border->Count; t++)
        {
            /* Add vector offset to current position */
            xoff = *ptr ++;
            yoff = *ptr ++;

            if (t == 0)
            {
                Move (rp, x + xoff, y + yoff);
            }
            else
            {
                /* Stroke */
                Draw (rp, x + xoff, y + yoff);
            }
        }

    } /* for ( ; border; border = border->NextBorder) */

    /* Restore RastPort */
#ifdef __MORPHOS__
    SetRPAttrs(rp,RPTAG_APen,apen,RPTAG_BPen,bpen,RPTAG_DrMd,drmd,RPTAG_PenMode,penmode,TAG_DONE);
#else
    SetRPAttrs(rp,RPTAG_APen,apen,RPTAG_BPen,bpen,RPTAG_DrMd,drmd,TAG_DONE);
#endif

    if (rp->Layer) UnlockLayer(rp->Layer);

    AROS_LIBFUNC_EXIT
} /* DrawBorder */
