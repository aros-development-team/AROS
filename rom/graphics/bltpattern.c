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

/****************************************************************************************/

struct bp_render_data
{
    HIDDT_PixelLUT   pixlut;
    UBYTE   	    *pattern;
    UBYTE   	    *mask;
    ULONG   	     maskmodulo;
    WORD    	     patternheight;
    WORD    	     patterndepth;
    WORD    	     renderx1;
    WORD    	     rendery1;
    UBYTE   	     invertpattern;
};

static ULONG bltpattern_render(APTR bpr_data, WORD srcx, WORD srcy,
    	    	    	       OOP_Object *dstbm_obj, OOP_Object *dst_gc,
    	    	    	       struct Rectangle *rect, struct GfxBase *GfxBase)
{
    struct bp_render_data *bprd = bpr_data;
    WORD		   width  = rect->MaxX - rect->MinX + 1;
    WORD		   height = rect->MaxY - rect->MinY + 1;
    UBYTE		  *mask = bprd->mask + bprd->maskmodulo * srcy;
    WORD		   patsrcx = (srcx + bprd->renderx1) % 16;
    WORD		   patsrcy = (srcy + bprd->rendery1) % bprd->patternheight;

    if (patsrcx < 0)
    	patsrcx += 16;
    if (patsrcy < 0)
    	patsrcy += bprd->patternheight;

    HIDD_BM_PutPattern(dstbm_obj, dst_gc, bprd->pattern,
    	    	       patsrcx, patsrcy, bprd->patternheight, bprd->patterndepth,
		       &bprd->pixlut, bprd->invertpattern, mask, bprd->maskmodulo,
		       srcx, rect->MinX, rect->MinY, width, height);

    return width * height;
}

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH7(void, BltPattern,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(PLANEPTR         , mask, A0),
	AROS_LHA(WORD             , xMin, D0),
	AROS_LHA(WORD             , yMin, D1),
	AROS_LHA(WORD             , xMax, D2),
	AROS_LHA(WORD             , yMax, D3),
	AROS_LHA(ULONG            , byteCnt, D4),

/*  LOCATION */
	struct GfxBase *, GfxBase, 52, Graphics)

/*  FUNCTION
	Blit using drawmode, areafill pattern and mask.

    INPUTS
	rp - destination RastPort for blit.
	mask - Mask bitplane. Set this to NULL for a rectangle.
	xMin, yMin - upper left corner.
	xMax, yMax - lower right corner.
	byteCnt - BytesPerRow for mask.

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

    if (rp->AreaPtrn)
    {
	struct bp_render_data 	bprd;
	struct Rectangle 	rr;

	FIX_GFXCOORD(xMin);
	FIX_GFXCOORD(yMin);

	bprd.pattern  	    = (UBYTE *)rp->AreaPtrn;
	bprd.mask   	    = mask;
	bprd.maskmodulo     = byteCnt;
	bprd.patterndepth   = (rp->AreaPtSz >= 0) ? 1 : rp->BitMap->Depth;
	bprd.patternheight  = 1L << ((rp->AreaPtSz >= 0) ? rp->AreaPtSz : -rp->AreaPtSz);
	bprd.renderx1	    = xMin;
	bprd.rendery1	    = yMin;
	bprd.invertpattern  = (rp->DrawMode & INVERSVID) ? TRUE : FALSE;
    	bprd.pixlut.entries = bprd.patterndepth;
	bprd.pixlut.pixels  = IS_HIDD_BM(rp->BitMap) ? HIDD_BM_PIXTAB(rp->BitMap) : NULL;
	
	rr.MinX = xMin;
	rr.MinY = yMin;
	rr.MaxX = xMax;
	rr.MaxY = yMax;

	do_render_func(rp, NULL, &rr, bltpattern_render, &bprd, TRUE, FALSE, GfxBase);
    }
    else
    {
    	if (mask)
	{
	    ULONG old_drawmode = GetDrMd(rp);
	    
	    if ((old_drawmode & ~INVERSVID) == JAM2)
	    	SetDrMd(rp, JAM1 | (old_drawmode & INVERSVID));

	    BltTemplate(mask, 0, byteCnt, rp, xMin, yMin, xMax - xMin + 1, yMax - yMin + 1);

	    SetDrMd(rp, old_drawmode);
	}
	else
	{
	    RectFill(rp, xMin, yMin, xMax, yMax);
	}
    }
    
    AROS_LIBFUNC_EXIT

} /* BltPattern */
