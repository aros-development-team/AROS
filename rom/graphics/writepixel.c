/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$	 $Log

    Desc: Graphics function WritePixel()
    Lang: english
*/

#include <graphics/rastport.h>
#include <graphics/clip.h>
#include <proto/graphics.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

struct pix_render_data
{
    HIDDT_Pixel pixel;
};

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

    INPUTS

    RESULT

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
        This function resembles very much the function WritePixel()!!

    HISTORY
	29-10-95    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    struct pix_render_data prd;
    LONG    	    	   retval;
    
    FIX_GFXCOORD(x);
    FIX_GFXCOORD(y);

    if(!OBTAIN_DRIVERDATA(rp, GfxBase))
	return  -1L;
	
    prd.pixel = BM_PIXEL(rp->BitMap, (UBYTE)rp->FgPen);
    retval = do_pixel_func(rp, x, y, pix_write, &prd, GfxBase);
    
    RELEASE_DRIVERDATA(rp, GfxBase);
    
    return retval;
     
    AROS_LIBFUNC_EXIT
} /* WritePixel */


static LONG pix_write(APTR pr_data, OOP_Object *bm, OOP_Object *gc,
    	    	      LONG x, LONG y, struct GfxBase *GfxBase)
{
    struct pix_render_data *prd;
    
    prd = (struct pix_render_data *)pr_data;
    
    HIDD_BM_PutPixel(bm, x, y, prd->pixel);
    
    return 0;
}
