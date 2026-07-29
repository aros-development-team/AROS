/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Graphics function RectFill()
*/

#include <proto/oop.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"
#include "graphics_driver.h"

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

        AROS_LH5(void, RectFill,

/*  SYNOPSIS */
        AROS_LHA(struct RastPort *, rp, A1),
        AROS_LHA(WORD             , xMin, D0),
        AROS_LHA(WORD             , yMin, D1),
        AROS_LHA(WORD             , xMax, D2),
        AROS_LHA(WORD             , yMax, D3),

/*  LOCATION */
        struct GfxBase *, GfxBase, 51, Graphics)

/*  FUNCTION
        Fills a rectangular area with the current pens, drawing mode
        and areafill pattern. If no areafill pattern is defined fill
        with foreground pen.

    INPUTS
        rp - RastPort
        xMin,yMin - upper left corner
        xMax,yMax - lower right corner

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
        29-10-95    digulla automatically created from
                            graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    FIX_GFXCOORD(xMin);
    FIX_GFXCOORD(yMin);
    FIX_GFXCOORD(xMax);
    FIX_GFXCOORD(yMax);

    if((xMax >= xMin) && (yMax >= yMin)) {
        if(rp->AreaPtrn) {
            /* When rastport has areaptrn, let BltPattern do the job */
            BltPattern(rp, NULL, xMin, yMin, xMax, yMax, 0);
        } else {
            /*
             * Without an AreaPtrn the fill pattern is all ones, so INVERSVID
             * clears it: JAM1 then writes nothing at all, and JAM2 writes
             * BPen everywhere. COMPLEMENT inverts the destination wherever
             * the mode writes, whichever pen it would have used.
             */
            BOOL inversvid = (rp->DrawMode & INVERSVID) != 0;
            BOOL jam2 = (rp->DrawMode & JAM2) != 0;

            if(jam2 || !inversvid) {
                OOP_Object *gc  = GetDriverData(rp, GfxBase);
                struct Rectangle rr;
                HIDDT_Pixel oldfg = GC_FG(gc);

                if(inversvid) {
                    GC_FG(gc) = GC_BG(gc);
                }
                if(rp->DrawMode & COMPLEMENT) {
                    GC_DRMD(gc) = vHidd_GC_DrawMode_Invert;
                }

                /* This is the same as fillrect_pendrmd() */

                rr.MinX = xMin;
                rr.MinY = yMin;
                rr.MaxX = xMax;
                rr.MaxY = yMax;

                do_render_with_gc(rp, NULL, &rr, fillrect_render, NULL, gc, TRUE, FALSE, GfxBase);

                GC_FG(gc) = oldfg;
            }
        }
    } /* if ((xMax >= xMin) && (yMax >= yMin)) */

    AROS_LIBFUNC_EXIT

} /* RectFill */
