/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
    $Id$        $Log

    Desc: Graphics function ScrollRaster()
*/
#include "graphics_intern.h"
#include <graphics/rastport.h>
#include "gfxfuncsupport.h"

#include <proto/graphics.h>

/*
 * Fill one strip that scrolling has vacated: through the layer's backfill
 * hook for ScrollRasterBF(), with RectFill() in the pens ScrollRaster() has
 * set up otherwise.
 */
static void scroll_fill(struct RastPort *rp, LONG x1, LONG y1, LONG x2, LONG y2,
                        BOOL backfill, struct GfxBase *GfxBase)
{
    if(backfill)
        EraseRect(rp, x1, y1, x2, y2);
    else
        RectFill(rp, x1, y1, x2, y2);
}

/*
 * The body of ScrollRaster() (backfill FALSE) and ScrollRasterBF() (backfill
 * TRUE), called with coordinates already passed through FIX_GFXCOORD().
 */
void internal_ScrollRaster(struct RastPort *rp, WORD dx, WORD dy, WORD xMin, WORD yMin,
                           WORD xMax, WORD yMax, BOOL backfill, struct GfxBase *GfxBase)
{
    UBYTE old_fgpen = rp->FgPen;
    UBYTE old_drmd = rp->DrawMode;
    LONG width, height, absdx, absdy;

    if((xMin > xMax) || (yMin > yMax)) return;

    /*
       adjust xMax and yMax in case the lower right corner would be outside
       the rastport
    */
    /* Is it a window's rastport ? */
    if(NULL != rp->Layer) {
        struct Layer *L = rp->Layer;

        if(xMax > (L->bounds.MaxX - L->bounds.MinX))
            xMax = (L->bounds.MaxX - L->bounds.MinX) ;

        if(yMax > (L->bounds.MaxY - L->bounds.MinY))
            yMax = (L->bounds.MaxY - L->bounds.MinY) ;

    } else {
        /* this one belongs to a screen */
        struct BitMap *bm = rp->BitMap;
        ULONG width  = GetBitMapAttr(bm, BMA_WIDTH);
        ULONG height = GetBitMapAttr(bm, BMA_HEIGHT);
        if((ULONG)xMax >= width)
            xMax = width - 1;
        if((ULONG)yMax >= height)
            yMax = height - 1;
    }

    absdx = (dx >= 0) ? dx : -dx;
    absdy = (dy >= 0) ? dy : -dy;

    width  = xMax - xMin + 1;
    height = yMax - yMin + 1;

    if((width < 1) || (height < 1)) return;

    /*
     * A distance that reaches the extent scrolls everything off. ScrollRaster()
     * then leaves the rectangle exactly as it was: not scrolled, and not
     * filled with BPen either. ScrollRasterBF() erases all of it.
     */
    if((absdx >= width) || (absdy >= height)) {
        if(backfill)
            EraseRect(rp, xMin, yMin, xMax, yMax);
        return;
    }

    if(!MoveRaster(rp, dx, dy, xMin, yMin, xMax, yMax, TRUE, GfxBase))
        return;

    /*
     * For ScrollRaster() the vacated strip is BPen whatever the caller's draw
     * mode: JAM1 with no AreaPtrn writes APen everywhere, and selecting it
     * drops COMPLEMENT and INVERSVID with it.
     */
    if(!backfill)
        SetABPenDrMd(rp, rp->BgPen, rp->BgPen, JAM1);

    /* was it scrolled left or right? */
    if(0 != dx) {
        if(dx > 0) {
            /* scrolled towards left, clearing on the right */
            scroll_fill(rp, xMax - dx + 1, yMin, xMax, yMax, backfill, GfxBase);
        } else {
            /* scrolled towards right, clearing on the left */
            /* a scroll by -1 should only erase a row of width 1 */
            scroll_fill(rp, xMin, yMin, xMin - dx - 1, yMax, backfill, GfxBase);
        }
    }

    if(0 != dy) {
        if(dy > 0) {
            /* scrolled up, clearing on the bottom */
            scroll_fill(rp, xMin, yMax - dy + 1, xMax, yMax, backfill, GfxBase);
        } else {
            /* scrolled down, clearing on the top */
            scroll_fill(rp, xMin, yMin, xMax, yMin - dy - 1, backfill, GfxBase);
        }
    }

    if(!backfill)
        SetABPenDrMd(rp, old_fgpen, rp->BgPen, old_drmd);
}

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

        AROS_LH7(void, ScrollRaster,

/*  SYNOPSIS */
        AROS_LHA(struct RastPort *, rp, A1),
        AROS_LHA(WORD             , dx, D0),
        AROS_LHA(WORD             , dy, D1),
        AROS_LHA(WORD             , xMin, D2),
        AROS_LHA(WORD             , yMin, D3),
        AROS_LHA(WORD             , xMax, D4),
        AROS_LHA(WORD             , yMax, D5),

/*  LOCATION */
        struct GfxBase *, GfxBase, 66, Graphics)

/*  FUNCTION
      Scroll the contents of a rastport (dx,dy) towards (0,0).
      The empty spaces is filled by a call to RectFill().
      Only the pixel in the rectangle (xMin,yMin)-(xMax,yMax)
      will be affected. The lower right corner (xMax, yMax) is
      automatically adjusted to the lower right corner in case
      it would be outside.
      After this operation the Flags bit of the layer associated
      with this rastport, if there is any layer, should be tested
      for simple layers in case there has any damage been created.


    INPUTS
      rp    - pointer to rastport
      dx,dy - distance to move in x and y direction. Positive values go
              towards (0,0)
      xMin,yMin - upper left  hand corner of the affected rectangle
      xMax,yMax - lower right hand corner of the affected rectangle

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

    internal_ScrollRaster(rp, dx, dy, xMin, yMin, xMax, yMax, FALSE, GfxBase);

    AROS_LIBFUNC_EXIT
} /* ScrollRaster */
