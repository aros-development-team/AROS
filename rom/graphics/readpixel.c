/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function ReadPixel()
    Lang: english
*/

#include <graphics/rastport.h>
#include <graphics/clip.h>
#include <proto/graphics.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

#include <aros/debug.h>

struct prlut8_render_data
{
    ULONG pen;
    HIDDT_PixelLUT *pixlut;
};

static LONG pix_read_lut8(APTR prlr_data, OOP_Object *bm, OOP_Object *gc,
	    	          LONG x, LONG y, struct GfxBase *GfxBase);

/*****************************************************************************

    NAME */

	AROS_LH3(LONG, ReadPixel,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(LONG             , x, D0),
	AROS_LHA(LONG             , y, D1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 53, Graphics)

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

    struct prlut8_render_data prlrd;
    LONG    	    	       ret;

    HIDDT_PixelLUT pixlut = { AROS_PALETTE_SIZE, HIDD_BM_PIXTAB(rp->BitMap) };

    FIX_GFXCOORD(x);
    FIX_GFXCOORD(y);
   
    if(!OBTAIN_DRIVERDATA(rp, GfxBase))
	return ((ULONG)-1L);
	
    if (IS_HIDD_BM(rp->BitMap))
    	prlrd.pixlut = &pixlut;
    else
    	prlrd.pixlut = NULL;
	
    prlrd.pen = -1;

    ret = do_pixel_func(rp, x, y, pix_read_lut8, &prlrd, GfxBase);
    
    RELEASE_DRIVERDATA(rp, GfxBase);
    
    if (-1 == ret || -1 == prlrd.pen)
    {
        D(bug("ReadPixel(), COULD NOT GET PEN. TRYING TO READ FROM SimpleRefresh cliprect ??"));
    	return (ULONG)-1;
    }
	
    return prlrd.pen;
 
    AROS_LIBFUNC_EXIT
  
} /* ReadPixel */

static LONG pix_read_lut8(APTR prlr_data, OOP_Object *bm, OOP_Object *gc,
    	    	    	  LONG x, LONG y, struct GfxBase *GfxBase)
{
    struct prlut8_render_data *prlrd;
       
    prlrd = (struct prlut8_render_data *)prlr_data;
    
    if (NULL != prlrd->pixlut)
    {
	HIDD_BM_GetImageLUT(bm, (UBYTE *)&prlrd->pen, 1, x, y, 1, 1, prlrd->pixlut);
    }
    else
    {
    	prlrd->pen = HIDD_BM_GetPixel(bm, x, y);
    }

    return 0;
}

