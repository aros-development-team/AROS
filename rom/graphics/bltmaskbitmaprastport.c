/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/debug.h>
#include <proto/graphics.h>
#include <proto/oop.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"
#include "graphics_driver.h"

/****************************************************************************************/

static ULONG bltmask_render(APTR bltmask_rd, WORD srcx, WORD srcy,
                            OOP_Object *dstbm_obj, OOP_Object *dst_gc,
                            struct Rectangle *rect, struct GfxBase *GfxBase)
{
    struct bitmap_render_data *brd = bltmask_rd;
    WORD width  = rect->MaxX - rect->MinX + 1;
    WORD height = rect->MaxY - rect->MinY + 1;
    OOP_Object *gfxhidd;
    BOOL ok;

    gfxhidd = SelectDriverObject(brd->srcbm, dstbm_obj, GfxBase);
    ok = HIDD_Gfx_CopyBoxMasked(gfxhidd, brd->srcbm_obj, srcx, srcy, dstbm_obj, rect->MinX, rect->MinY, width, height,
                                brd->mask, dst_gc);

    return ok ? width * height : 0;
}

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

        AROS_LH10(void, BltMaskBitMapRastPort,

/*  SYNOPSIS */
        AROS_LHA(struct BitMap   *, srcBitMap, A0),
        AROS_LHA(WORD             , xSrc, D0),
        AROS_LHA(WORD             , ySrc, D1),
        AROS_LHA(struct RastPort *, destRP, A1),
        AROS_LHA(WORD             , xDest, D2),
        AROS_LHA(WORD             , yDest, D3),
        AROS_LHA(WORD             , xSize, D4),
        AROS_LHA(WORD             , ySize, D5),
        AROS_LHA(UBYTE            , minterm, D6),
        AROS_LHA(PLANEPTR         , bltMask, A2),

/*  LOCATION */
        struct GfxBase *, GfxBase, 106, Graphics)

/*  FUNCTION
        Copies a part of a bitmap to another bitmap with using a mask.

    INPUTS
        srcBitMap - Copy from this bitmap.
        xSrc, ySrc - This is the upper left corner of the area to copy.
        destRP - Destination RastPort.
        xDest, yDest - Upper left corner where to place the copy
        xSize, ySize - The size of the area to copy
        minterm - How to copy. See BltBitMap() for an explanation.
        bltMask - The mask bitplane must be of the same size as the source bitmap.

    RESULT
        None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        ClipBlit()

    INTERNALS

    HISTORY
        27-11-96    digulla automatically created from
                            graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    EnterFunc(bug("BltMaskBitMapRastPort(%p (%d*%d), %d*%d, %p, %d*%d, %d*%d, %02x, %p)\n",
                  srcBitMap, srcBitMap->BytesPerRow, srcBitMap->Rows, xSrc, ySrc,
                  destRP, xDest, yDest, xSize, ySize, minterm, bltMask));

    FIX_GFXCOORD(xSrc);
    FIX_GFXCOORD(ySrc);
    FIX_GFXCOORD(xDest);
    FIX_GFXCOORD(yDest);

    if((xSize < 1) || (ySize < 1)) return;

    internal_BltBitMapRastPort(srcBitMap, xSrc, ySrc, destRP, xDest, yDest, xSize, ySize, minterm,
                               bltmask_render, bltMask, GfxBase);

    ReturnVoid("BltBitMapRastPort");

    AROS_LIBFUNC_EXIT

} /* BltMaskBitMapRastPort */
