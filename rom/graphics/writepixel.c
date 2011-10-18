/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$	 $Log

    Desc: Graphics function WritePixel()
    Lang: english
*/

#include <graphics/rastport.h>
#include <graphics/clip.h>
#include <proto/graphics.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

static LONG pix_write(APTR pr_data, OOP_Object *bm, OOP_Object *gc,
    	    	      LONG x, LONG y, struct GfxBase *GfxBase);

/*****************************************************************************

    NAME */

	AROS_LH3(LONG, WritePixel,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(LONG             , x, D0),
	AROS_LHA(LONG             , y, D1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 54, Graphics)

/*  FUNCTION
	Change pen number of a pixel at given coordinate.
	The pixel is drawn with the primary (A) pen.

    INPUTS
	rp  - destination RastPort
	x,y - coordinate

    RESULT
	 0: pixel could be written
	-1: coordinate was outside rastport

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        This function takes layers into account. Some pixel that is
        being read is not found on the display-bitmap but in some
        clipped rectangle (cliprect) in a layer structure.
        There is no support of anything else than bitplanes now.
        (No chunky pixels)

    HISTORY
	29-10-95    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    HIDDT_Pixel pix;
    
    FIX_GFXCOORD(x);
    FIX_GFXCOORD(y);

    pix = BM_PIXEL(rp->BitMap, (UBYTE)rp->FgPen);

    return do_pixel_func(rp, x, y, pix_write, (APTR)pix, TRUE, GfxBase);

    AROS_LIBFUNC_EXIT
} /* WritePixel */


static LONG pix_write(APTR pr_data, OOP_Object *bm, OOP_Object *gc,
    	    	      LONG x, LONG y, struct GfxBase *GfxBase)
{   
    HIDD_BM_PutPixel(bm, x, y, (HIDDT_Pixel)pr_data);

    return 0;
}
