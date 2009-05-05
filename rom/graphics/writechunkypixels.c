/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/debug.h>
#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH7(void, WriteChunkyPixels,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A0),
	AROS_LHA(LONG             , xstart, D0),
	AROS_LHA(LONG             , ystart, D1),
	AROS_LHA(LONG             , xstop, D2),
	AROS_LHA(LONG             , ystop, D3),
	AROS_LHA(UBYTE           *, array, A2),
	AROS_LHA(LONG             , bytesperrow, D4),

/*  LOCATION */
	struct GfxBase *, GfxBase, 176, Graphics)

/*  FUNCTION
	Write a rectangular region of pen values into a rastport.

    INPUTS
	rp            - destination RastPort
	xstart,ystart - starting point
	xstop,ystop   - stopping point
	array         - array with pen values
	bytesperrow   - The number of bytes per row in the source array.
		        This should be at least as large as the number of pixels
		        being written per line.

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

    HIDDT_PixelLUT pixlut;

    pixlut.entries = AROS_PALETTE_SIZE;
    pixlut.pixels  = IS_HIDD_BM(rp->BitMap) ? HIDD_BM_PIXTAB(rp->BitMap) : NULL;

    FIX_GFXCOORD(xstart);
    FIX_GFXCOORD(ystart);
    FIX_GFXCOORD(xstop);
    FIX_GFXCOORD(ystop);
    
    if ((xstart > xstop) || (ystart > ystop)) return;

    if (!pixlut.pixels)
    {
    	if (GetBitMapAttr(rp->BitMap, BMA_DEPTH) > 8)
	{
	    D(bug("WriteChunkyPixels: can't work on hicolor/truecolor screen without LUT"));
    	    return;
	}
    }
      
    write_pixels_8(rp, array
    	, bytesperrow
	, xstart, ystart
	, xstop, ystop
	, &pixlut
        , TRUE
	, GfxBase);

    AROS_LIBFUNC_EXIT
    
} /* WriteChunkyPixels */
