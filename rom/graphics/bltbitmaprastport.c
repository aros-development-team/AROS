/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/debug.h>
#include <proto/oop.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"
#include "graphics_driver.h"

/****************************************************************************************/

struct bitmap_render_data
{
    struct render_special_info rsi;
    ULONG   	    	       minterm;
    struct BitMap   	      *srcbm;
    OOP_Object      	      *srcbm_obj;
};

static ULONG bitmap_render(APTR bitmap_rd, WORD srcx, WORD srcy,
    	    	    	   OOP_Object *dstbm_obj, OOP_Object *dst_gc,
    	    	    	   struct Rectangle *rect, struct GfxBase *GfxBase)
{
    struct bitmap_render_data *brd = bitmap_rd;
    OOP_Object		      *gfxhidd = SelectDriverObject(brd->srcbm, dstbm_obj, GfxBase);
    WORD		       width  = rect->MaxX - rect->MinX + 1;
    WORD		       height = rect->MaxY - rect->MinY + 1;
    BOOL		       res;

//    D(bug("bitmap_render(%p, %d, %d, %p, %p, %d, %d, %d, %d, %p)\n"
//	, bitmap_rd, srcx, srcy, dstbm_obj, dst_gc, rect->MinX, rect->MinY, rect->MaxX, rect->MaxY, GfxBase));

    /*
     * Get some info on the colormaps. We have to make sure
     * that we have the appropriate mapping tables set.
     */
    res = int_bltbitmap(brd->srcbm, brd->srcbm_obj, srcx, srcy,
    			brd->rsi.curbm, dstbm_obj, rect->MinX, rect->MinY,
    			width, height, brd->minterm, gfxhidd, dst_gc, GfxBase);

   return res ? width * height : 0;
}

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH9 (void, BltBitMapRastPort,

/*  SYNOPSIS */
	AROS_LHA(struct BitMap   *, srcBitMap, A0),
	AROS_LHA(WORD             , xSrc, D0),
	AROS_LHA(WORD             , ySrc, D1),
	AROS_LHA(struct RastPort *, destRP, A1),
	AROS_LHA(WORD             , xDest, D2),
	AROS_LHA(WORD             , yDest, D3),
	AROS_LHA(WORD             , xSize, D4),
	AROS_LHA(WORD             , ySize, D5),
	AROS_LHA(ULONG            , minterm, D6),

/*  LOCATION */
	struct GfxBase *, GfxBase, 101, Graphics)
	    
/*  FUNCTION
	Moves part of a bitmap around or into another bitmap.

    INPUTS
	srcBitMap - Copy from this bitmap.
	xSrc, ySrc - This is the upper left corner of the area to copy.
	destRP - Destination RastPort.
	xDest, yDest - Upper left corner where to place the copy
	xSize, ySize - The size of the area to copy
	minterm - How to copy. See BltBitMap() for an explanation.

    RESULT
	TRUE.

    NOTES
	If special hardware is available, this function will use it.

    EXAMPLE

    BUGS

    SEE ALSO
	ClipBlit()

    INPUTS

    RESULT

    NOTES

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
     
    struct bitmap_render_data 	brd;
    struct Rectangle 	    	rr;
    OOP_Object      	    	*gc;
    Point   	    	    	src;

    EnterFunc(bug("BltBitMapRastPort(%d %d %d, %d, %d, %d, %x, %p, %p)\n"
    	, xSrc, ySrc, xDest, yDest, xSize, ySize, minterm, srcBitMap, destRP));

    FIX_GFXCOORD(xSrc);
    FIX_GFXCOORD(ySrc);
    FIX_GFXCOORD(xDest);
    FIX_GFXCOORD(yDest);

    brd.srcbm_obj = OBTAIN_HIDD_BM(srcBitMap);
    if (NULL == brd.srcbm_obj)
    {
    	return;
    }

    brd.minterm	= minterm;
    brd.srcbm = srcBitMap;

    /* Get RastPort's GC and set the correct ROP */
    gc = GetDriverData(destRP, GfxBase);
    GC_DRMD(gc) = MINTERM_TO_GCDRMD(minterm);

    rr.MinX = xDest;
    rr.MinY = yDest;
    rr.MaxX = xDest + xSize - 1;
    rr.MaxY = yDest + ySize - 1;

    src.x = xSrc;
    src.y = ySrc;

    do_render_with_gc(destRP, &src, &rr, bitmap_render, &brd, gc, TRUE, TRUE, GfxBase);

    RELEASE_HIDD_BM(brd.srcbm_obj, srcBitMap);
    ReturnVoid("BltBitMapRastPort");

    AROS_LIBFUNC_EXIT

} /* BltBitMapRastPort */
