/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/debug.h>
#include "graphics_intern.h"
#include "gfxfuncsupport.h"

struct rp8_render_data
{
    UBYTE *array;
    ULONG modulo;
    HIDDT_PixelLUT *pixlut;
};

static ULONG rp8_render(APTR rp8r_data, LONG srcx, LONG srcy,
    	    	    	OOP_Object *srcbm_obj, OOP_Object *gc,
			LONG x1, LONG y1, LONG x2, LONG y2,
			struct GfxBase *GfxBase);

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH7(LONG, ReadPixelArray8,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *	, rp		, A0),
	AROS_LHA(LONG             	, xstart	, D0),
	AROS_LHA(LONG             	, ystart	, D1),
	AROS_LHA(LONG             	, xstop		, D2),
	AROS_LHA(LONG             	, ystop		, D3),
	AROS_LHA(UBYTE * 		, array		, A2),
	AROS_LHA(struct RastPort *	, temprp	, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 130, Graphics)

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

    struct rp8_render_data  rp8rd;
    struct Rectangle 	    rr;
    HIDDT_PixelLUT  	    pixlut;  
    LONG    	    	    pixread = 0;
    
    EnterFunc(bug("ReadPixelArray8(%p, %d, %d, %d, %d)\n",
    	rp, xstart, ystart, xstop, ystop));
    
    FIX_GFXCOORD(xstart);
    FIX_GFXCOORD(ystart);
    FIX_GFXCOORD(xstop);
    FIX_GFXCOORD(ystop);
    
    if ((xstart > xstop) || (ystart > ystop)) return 0;
    
    if (!OBTAIN_DRIVERDATA(rp, GfxBase))
	return 0;
	
#warning "ReadPixelArray8 on hi/truecolor screens or a LUT for it does not really make sense"

    pixlut.entries = AROS_PALETTE_SIZE;
    pixlut.pixels  = IS_HIDD_BM(rp->BitMap) ? HIDD_BM_PIXTAB(rp->BitMap) : NULL;
     
    rp8rd.array  = array;
    rp8rd.modulo = ((xstop - xstart + 1) + 15) & ~15;
    rp8rd.pixlut = &pixlut;
    
    rr.MinX = xstart;
    rr.MinY = ystart;
    rr.MaxX = xstop;
    rr.MaxY = ystop;
    
    pixread = do_render_func(rp, NULL, &rr, rp8_render, &rp8rd, FALSE, GfxBase);
	
    RELEASE_DRIVERDATA(rp, GfxBase);
    
    ReturnInt("ReadPixelArray8", LONG, pixread);

    AROS_LIBFUNC_EXIT

} /* ReadPixelArray8 */

/****************************************************************************************/

static ULONG rp8_render(APTR rp8r_data, LONG srcx, LONG srcy,
    	    	        OOP_Object *srcbm_obj, OOP_Object *gc,
			LONG x1, LONG y1, LONG x2, LONG y2,
			struct GfxBase *GfxBase)
{
    struct rp8_render_data *rp8rd;
    ULONG   	    	    width, height;
    
    rp8rd = (struct rp8_render_data *)rp8r_data;
    
    width  = x2 - x1 + 1;
    height = y2 - y1 + 1;
    
    HIDD_BM_GetImageLUT(srcbm_obj
	, rp8rd->array + CHUNKY8_COORD_TO_BYTEIDX(srcx, srcy, rp8rd->modulo)
	, rp8rd->modulo
	, x1, y1
	, width, height
	, rp8rd->pixlut
    );
    
    return width * height;
}

/****************************************************************************************/
