/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <hidd/gfx.h>
#include <aros/debug.h>

#include "cybergraphics_intern.h"
#include "gfxfuncsupport.h"

struct render_data
{
    HIDDT_Color col;
};

static ULONG RenderHook(struct render_data *data, LONG srcx, LONG srcy,
    OOP_Object *dstbm_obj, OOP_Object *dst_gc, struct Rectangle *rect,
    struct GfxBase *GfxBase);

/*****************************************************************************

    NAME */
#include <proto/cybergraphics.h>

        AROS_LH6(ULONG, FillPixelArray,

/*  SYNOPSIS */
        AROS_LHA(struct RastPort *, rp          , A1),
        AROS_LHA(UWORD            , destx       , D0),
        AROS_LHA(UWORD            , desty       , D1),
        AROS_LHA(UWORD            , width       , D2),
        AROS_LHA(UWORD            , height      , D3),
        AROS_LHA(ULONG            , pixel       , D4),

/*  LOCATION */
        struct Library *, CyberGfxBase, 25, Cybergraphics)

/*  FUNCTION
        Writes the same color value to all pixels in a rectangular region of
        a RastPort.

    INPUTS
        rp - the RastPort to write to.
        destx, desty - top-lefthand corner of portion of destination RastPort
            to write to (in pixels).
        width, height - size of the affected area (in pixels).
        pixel - the color value to use, in 32-bit ARGB format: 1 byte per
            component, in the order alpha, red, green, blue.

    RESULT
        count - the number of pixels filled.

    NOTES
        The colour is mapped for each bitmap actually written: a composited
        layer's alpha surface (LA_AlphaSurface) on a screen without an alpha
        channel receives the alpha given here, although the RastPort's
        bitmap (the screen's) cannot hold it.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct render_data data;
    struct Rectangle rr;

    if (width == 0 || height == 0)
        return 0;

    /* HIDDT_ColComp are 16 Bit */
    data.col.alpha   = (HIDDT_ColComp)((pixel >> 16) & 0x0000FF00);
    data.col.red     = (HIDDT_ColComp)((pixel >> 8) & 0x0000FF00);
    data.col.green   = (HIDDT_ColComp)(pixel & 0x0000FF00);
    data.col.blue    = (HIDDT_ColComp)((pixel << 8) & 0x0000FF00);

    rr.MinX = destx;
    rr.MinY = desty;
    rr.MaxX = destx + width  - 1;
    rr.MaxY = desty + height - 1;

    return (LONG)DoRenderFunc(rp, NULL, &rr, RenderHook, &data, TRUE);

    AROS_LIBFUNC_EXIT
} /* FillPixelArray */

/*
 * Mapped per destination: the pixel value the RastPort's bitmap would give
 * loses alpha when the destination is a layer surface with an alpha channel
 * of its own. The GC belongs to the RastPort, so its colour is put back.
 */
static ULONG RenderHook(struct render_data *data, LONG srcx, LONG srcy,
    OOP_Object *dstbm_obj, OOP_Object *dst_gc, struct Rectangle *rect,
    struct GfxBase *GfxBase)
{
    HIDDT_Pixel oldfg = GC_FG(dst_gc);
    UBYTE olddrmd = GC_DRMD(dst_gc);

    GC_FG(dst_gc)   = HIDD_BM_MapColor(dstbm_obj, &data->col);
    GC_DRMD(dst_gc) = vHidd_GC_DrawMode_Copy;

    HIDD_BM_FillRect(dstbm_obj, dst_gc, rect->MinX, rect->MinY, rect->MaxX, rect->MaxY);

    GC_FG(dst_gc)   = oldfg;
    GC_DRMD(dst_gc) = olddrmd;

    return (rect->MaxX - rect->MinX + 1) * (rect->MaxY - rect->MinY + 1);
}
