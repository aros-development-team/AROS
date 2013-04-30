/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function RectFill()
    Lang: english
*/

#include <proto/oop.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"
#include "graphics_driver.h"

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH5(void, RectFill,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(LONG             , xMin, D0),
	AROS_LHA(LONG             , yMin, D1),
	AROS_LHA(LONG             , xMax, D2),
	AROS_LHA(LONG             , yMax, D3),

/*  LOCATION */
	struct GfxBase *, GfxBase, 51, Graphics)

/*  FUNCTION
	Fills a rectangular area with the current pens, drawing mode
	and areafill pattern. If no areafill pattern is defined fill
	with foreground pen.

    INPUTS
	rp - RastPort
	xMin,yMin - upper left corner
	xMax,yMax - lower right corner

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    FIX_GFXCOORD(xMin);
    FIX_GFXCOORD(yMin);
    FIX_GFXCOORD(xMax);
    FIX_GFXCOORD(yMax);

    if ((xMax >= xMin) && (yMax >= yMin))
    {
	if (rp->AreaPtrn)
	{
    	    /* When rastport has areaptrn, let BltPattern do the job */
	    BltPattern(rp, NULL, xMin, yMin, xMax, yMax, 0);
	}
	else
	{
	    OOP_Object *gc  = GetDriverData(rp, GfxBase);
	    struct Rectangle rr;
	    HIDDT_Pixel oldfg = 0;

	    if (rp->DrawMode & INVERSVID) {
	        oldfg = GC_FG(gc);
	        GC_FG(gc) = GC_BG(gc);
	    }

	    /* This is the same as fillrect_pendrmd() */

	    rr.MinX = xMin;
	    rr.MinY = yMin;
	    rr.MaxX = xMax;
	    rr.MaxY = yMax;

    	    do_render_with_gc(rp, NULL, &rr, fillrect_render, NULL, gc, TRUE, FALSE, GfxBase);

	    if (rp->DrawMode & INVERSVID) {
	        GC_FG(gc) = oldfg;
	    }
    	}
    } /* if ((xMax >= xMin) && (yMax >= yMin)) */
    
    AROS_LIBFUNC_EXIT

} /* RectFill */
