/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function WritePixel()
    Lang: english
*/

#include <graphics/rastport.h>
#include <graphics/clip.h>
#include <proto/graphics.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

static LONG pix_write(APTR pr_data, OOP_Object *bm, OOP_Object *gc,
    	    	      WORD x, WORD y, struct GfxBase *GfxBase);

/*****************************************************************************

    NAME */

	AROS_LH3(LONG, WritePixel,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(WORD             , x, D0),
	AROS_LHA(WORD             , y, D1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 54, Graphics)

/*  FUNCTION
	Write the primary (A) pen colour to the given coordinates of a
	RastPort.

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

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    IPTR pix;

    FIX_GFXCOORD(x);
    FIX_GFXCOORD(y);

    if ((rp->Flags & RPF_NO_PENS) != 0)
        pix = RP_FGCOLOR(rp);
    else
        pix = BM_PIXEL(rp->BitMap, (UBYTE)rp->FgPen);

    return do_pixel_func(rp, x, y, pix_write, (APTR)pix, TRUE, GfxBase);

    AROS_LIBFUNC_EXIT
} /* WritePixel */


static LONG pix_write(APTR pr_data, OOP_Object *bm, OOP_Object *gc,
    	    	      WORD x, WORD y, struct GfxBase *GfxBase)
{
    HIDD_BM_PutPixel(bm, x, y, (IPTR)pr_data);

    return 0;
}
