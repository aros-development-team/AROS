/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/debug.h>
#include <proto/oop.h>
#include "graphics_intern.h"
#include "gfxfuncsupport.h"

struct bitmap_render_data
{
    struct render_special_info rsi;
    ULONG   	    	       minterm;
    struct BitMap   	       *srcbm;
    OOP_Object      	       *srcbm_obj;

};

static ULONG bitmap_render(APTR bitmap_rd, LONG srcx, LONG srcy,
    	    	    	   OOP_Object *dstbm_obj, OOP_Object *dst_gc,
			   LONG x1, LONG y1, LONG x2, LONG y2, struct GfxBase *GfxBase);

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH9 (void, BltBitMapRastPort,

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

/*  LOCATION */
	struct GfxBase *, GfxBase, 101, Graphics)
	    
/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
     
    struct bitmap_render_data 	brd;
    struct Rectangle 	    	rr;
    HIDDT_DrawMode  	    	old_drmd;
    OOP_Object      	    	*gc;
    Point   	    	    	src;

    struct TagItem gc_tags[] =
    {
    	{ aHidd_GC_DrawMode , 0UL },
	{ TAG_DONE  	    	  }
    };

    EnterFunc(bug("BltBitMapRastPort(%d %d %d, %d, %d, %d)\n"
    	, xSrc, ySrc, xDest, yDest, xSize, ySize));

    if (!OBTAIN_DRIVERDATA(destRP, GfxBase))
    	return;

    FIX_GFXCOORD(xSrc);
    FIX_GFXCOORD(ySrc);
    FIX_GFXCOORD(xDest);
    FIX_GFXCOORD(yDest);
    
    brd.minterm	= minterm;
    brd.srcbm_obj = OBTAIN_HIDD_BM(srcBitMap);
    if (NULL == brd.srcbm_obj)
    {
    	RELEASE_DRIVERDATA(destRP, GfxBase);
    	return;
    }
    
    brd.srcbm = srcBitMap;

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

    do_render_func(destRP, &src, &rr, bitmap_render, &brd, TRUE, GfxBase);

    RELEASE_HIDD_BM(brd.srcbm_obj, srcBitMap);

    gc_tags[0].ti_Data = old_drmd;
    OOP_SetAttrs(gc, gc_tags);

    RELEASE_DRIVERDATA(destRP, GfxBase);
    
    ReturnVoid("BltBitMapRastPort");

    AROS_LIBFUNC_EXIT
    
} /* BltBitMapRastPort */

/****************************************************************************************/

static ULONG bitmap_render(APTR bitmap_rd, LONG srcx, LONG srcy,
    	    	    	   OOP_Object *dstbm_obj, OOP_Object *dst_gc,
			   LONG x1, LONG y1, LONG x2, LONG y2, struct GfxBase *GfxBase)
{
    struct bitmap_render_data 	*brd;
    ULONG   	    	    	width, height;

    width  = x2 - x1 + 1;
    height = y2 - y1 + 1;

    brd = (struct bitmap_render_data *)bitmap_rd;

//    D(bug("bitmap_render(%p, %d, %d, %p, %p, %d, %d, %d, %d, %p)\n"
//	, bitmap_rd, srcx, srcy, dstbm_obj, dst_gc, x1, y1, x2, y2, GfxBase));


    /* Get some info on the colormaps. We have to make sure
       that we have the apropriate mapping tables set.
    */

    if (!int_bltbitmap(brd->srcbm
    	, brd->srcbm_obj
	, srcx, srcy
	, brd->rsi.curbm
	, dstbm_obj
	, x1, y1
	, x2 - x1 + 1, y2 - y1 + 1
	, brd->minterm
	, dst_gc
	, GfxBase
    ))
    	return 0;

   return width * height;
}

/****************************************************************************************/
