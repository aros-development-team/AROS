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

struct bltmask_render_data
{
    struct render_special_info rsi;
    struct BitMap   	       *srcbm;
    OOP_Object      	       *srcbm_obj;
    PLANEPTR	    	       mask;
};

static ULONG bltmask_render(APTR bltmask_rd, LONG srcx, LONG srcy,
    	    	    	    OOP_Object *dstbm_obj, OOP_Object *dst_gc,
			    LONG x1, LONG y1, LONG x2, LONG y2, struct GfxBase *GfxBase);

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
    HIDDT_DrawMode  	    	old_drmd;
    OOP_Object      	    	*gc;
    Point   	    	    	src;

    struct TagItem gc_tags[] =
    {
    	{ aHidd_GC_DrawMode , 0UL },
	{ TAG_DONE  	    	  }
    };

    EnterFunc(bug("BltMaskBitMapRastPort(%d %d %d, %d, %d, %d)\n"
    	, xSrc, ySrc, xDest, yDest, xSize, ySize));

    FIX_GFXCOORD(xSrc);
    FIX_GFXCOORD(ySrc);
    FIX_GFXCOORD(xDest);
    FIX_GFXCOORD(yDest);
    
    if ((xSize < 1) || (ySize < 1)) return;
    
    if (!OBTAIN_DRIVERDATA(destRP, GfxBase))
    	return;

    brd.srcbm_obj = OBTAIN_HIDD_BM(srcBitMap);
    if (NULL == brd.srcbm_obj)
    {
    	RELEASE_DRIVERDATA(destRP, GfxBase);
    	return;
    }
    
    brd.srcbm = srcBitMap;
    brd.mask  = bltMask;

    gc = GetDriverData(destRP)->dd_GC;
    OOP_GetAttr(gc, aHidd_GC_DrawMode, &old_drmd);

    gc_tags[0].ti_Data = MINTERM_TO_GCDRMD(minterm);
    OOP_SetAttrs(gc, gc_tags);

    rr.MinX = xDest;
    rr.MinY = yDest;
    rr.MaxX = xDest + xSize - 1;
    rr.MaxY = yDest + ySize - 1;

    src.x = xSrc;
    src.y = ySrc;

    do_render_func(destRP, &src, &rr, bltmask_render, &brd, TRUE, TRUE, GfxBase);

    RELEASE_HIDD_BM(brd.srcbm_obj, srcBitMap);

    gc_tags[0].ti_Data = old_drmd;
    OOP_SetAttrs(gc, gc_tags);

    RELEASE_DRIVERDATA(destRP, GfxBase);
    
    ReturnVoid("BltBitMapRastPort");
    
    AROS_LIBFUNC_EXIT
    
} /* BltMaskBitMapRastPort */

/****************************************************************************************/

static ULONG bltmask_render(APTR bltmask_rd, LONG srcx, LONG srcy,
    	    	    	    OOP_Object *dstbm_obj, OOP_Object *dst_gc,
			    LONG x1, LONG y1, LONG x2, LONG y2, struct GfxBase *GfxBase)
{
    struct bltmask_render_data *brd = bltmask_rd;
    ULONG width  = x2 - x1 + 1;
    ULONG height = y2 - y1 + 1;
    OOP_Object *gfxhidd;
    BOOL ok;

    gfxhidd = SelectDriverObject(brd->srcbm, dstbm_obj, GfxBase);
    ok = HIDD_Gfx_CopyBoxMasked(gfxhidd, brd->srcbm_obj, srcx, srcy, dstbm_obj, x1, y1, width, height, brd->mask, dst_gc);

    return ok ? width * height : 0;
}

/****************************************************************************************/
