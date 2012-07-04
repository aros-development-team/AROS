/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/debug.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/****************************************************************************************/


struct rp8_render_data
{
    UBYTE *array;
    ULONG modulo;
    HIDDT_PixelLUT *pixlut;
};

static ULONG rp8_render(APTR rp8r_data, WORD srcx, WORD srcy,
    	    	        OOP_Object *srcbm_obj, OOP_Object *gc,
    	    	        struct Rectangle *rect, struct GfxBase *GfxBase)
{
    struct rp8_render_data *rp8rd  = rp8r_data;
    WORD   	    	    width  = rect->MaxX - rect->MinX + 1;
    WORD		    height = rect->MaxY - rect->MinY + 1;
    
    HIDD_BM_GetImageLUT(srcbm_obj, rp8rd->array + CHUNKY8_COORD_TO_BYTEIDX(srcx, srcy, rp8rd->modulo), rp8rd->modulo,
    			rect->MinX, rect->MinY, width, height, rp8rd->pixlut);

    return width * height;
}

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH7(LONG, ReadPixelArray8,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *	, rp		, A0),
	AROS_LHA(WORD             	, xstart	, D0),
	AROS_LHA(WORD             	, ystart	, D1),
	AROS_LHA(WORD             	, xstop		, D2),
	AROS_LHA(WORD             	, ystop		, D3),
	AROS_LHA(UBYTE * 		, array		, A2),
	AROS_LHA(struct RastPort *	, temprp	, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 130, Graphics)

/*  FUNCTION
	Read the pen numbers of a rectangular area into an array.

    INPUTS
	rp            - RastPort
	xstart,ystart - starting point
	xstop,ystop   - stopping point
	array         - array where pens are stored. Allocate at least
	                (((width+15)>>4)<<4)*(ystop-ystart+1) bytes.
	temprp        - temporary RastPort; copy of rp with
	                - Layers == NULL
	                - temprp->BitMap with Rows set to 1,
	                - temprp->BytesPerRow set to (((width+15)>>4)<<1),
	                  and temporary memory allocated for 
	                  temprp->BitMap->Planes[])

    RESULT
	The number of pixels read.

    NOTES
	This function doesn't make sense on true-/hicolor rastports.

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

    /* FIXME: ReadPixelArray8 on hi/truecolor screens or
     * a LUT for it does not really make sense
     */

    pixlut.entries = AROS_PALETTE_SIZE;
    pixlut.pixels  = IS_HIDD_BM(rp->BitMap) ? HIDD_BM_PIXTAB(rp->BitMap) : NULL;
     
    rp8rd.array  = array;
    rp8rd.modulo = ((xstop - xstart + 1) + 15) & ~15;
    rp8rd.pixlut = &pixlut;
    
    rr.MinX = xstart;
    rr.MinY = ystart;
    rr.MaxX = xstop;
    rr.MaxY = ystop;

    pixread = do_render_func(rp, NULL, &rr, rp8_render, &rp8rd, FALSE, FALSE, GfxBase);

    ReturnInt("ReadPixelArray8", LONG, pixread);

    AROS_LIBFUNC_EXIT

} /* ReadPixelArray8 */
