/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/debug.h>
#include <proto/graphics.h>
#include <proto/oop.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"
#include "graphics_driver.h"

/****************************************************************************************/

struct bltmask_render_data
{
    struct render_special_info rsi;
    struct BitMap   	       *srcbm;
    OOP_Object      	       *srcbm_obj;
    PLANEPTR	    	       mask;
};

static ULONG bltmask_render(APTR bltmask_rd, LONG srcx, LONG srcy,
    	    	    	    OOP_Object *dstbm_obj, OOP_Object *dst_gc,
    	    	    	    struct Rectangle *rect, struct GfxBase *GfxBase)
{
    struct bltmask_render_data *brd = bltmask_rd;
    ULONG width  = rect->MaxX - rect->MinX + 1;
    ULONG height = rect->MaxY - rect->MinY + 1;
    OOP_Object *gfxhidd;
    BOOL ok;

    gfxhidd = SelectDriverObject(brd->srcbm, dstbm_obj, GfxBase);
    ok = HIDD_Gfx_CopyBoxMasked(gfxhidd, brd->srcbm_obj, srcx, srcy, dstbm_obj, rect->MinX, rect->MinY, width, height, brd->mask, dst_gc);

    return ok ? width * height : 0;
}

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH10(void, BltMaskBitMapRastPort,

/*  SYNOPSIS */
	AROS_LHA(struct BitMap   *, srcBitMap, A0),
	AROS_LHA(LONG             , xSrc, D0),
	AROS_LHA(LONG             , ySrc, D1),
	AROS_LHA(struct RastPort *, destRP, A1),
	AROS_LHA(LONG             , xDest, D2),
	AROS_LHA(LONG             , yDest, D3),
	AROS_LHA(LONG             , xSize, D4),
	AROS_LHA(LONG             , ySize, D5),
	AROS_LHA(ULONG            , minterm, D6),
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
	TRUE.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	ClipBlit()

    INPUTS

    RESULT

    NOTES

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    struct bltmask_render_data 	brd;
    struct Rectangle 	    	rr;
    OOP_Object      	    	*gc;
    Point   	    	    	src;

    EnterFunc(bug("BltMaskBitMapRastPort(%p (%d*%d), %d*%d, %p, %d*%d, %d*%d, %02x, %p)\n",
        srcBitMap, srcBitMap->BytesPerRow, srcBitMap->Rows, xSrc, ySrc,
        destRP, xDest, yDest, xSize, ySize, minterm, bltMask));

    FIX_GFXCOORD(xSrc);
    FIX_GFXCOORD(ySrc);
    FIX_GFXCOORD(xDest);
    FIX_GFXCOORD(yDest);

    if ((xSize < 1) || (ySize < 1)) return;

    brd.srcbm_obj = OBTAIN_HIDD_BM(srcBitMap);
    if (NULL == brd.srcbm_obj)
    {
    	return;
    }

    brd.srcbm = srcBitMap;
    brd.mask  = bltMask;

    gc = GetDriverData(destRP, GfxBase);
    GC_DRMD(gc) = MINTERM_TO_GCDRMD(minterm);

    rr.MinX = xDest;
    rr.MinY = yDest;
    rr.MaxX = xDest + xSize - 1;
    rr.MaxY = yDest + ySize - 1;

    src.x = xSrc;
    src.y = ySrc;

    do_render_with_gc(destRP, &src, &rr, bltmask_render, &brd, gc, TRUE, TRUE, GfxBase);

    RELEASE_HIDD_BM(brd.srcbm_obj, srcBitMap);    
    ReturnVoid("BltBitMapRastPort");

    AROS_LIBFUNC_EXIT
    
} /* BltMaskBitMapRastPort */
