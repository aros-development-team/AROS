/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$    $Log

    Desc: Graphics function RectFill()
    Lang: english
*/
#include "graphics_intern.h"
#include "gfxfuncsupport.h"

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

    INPUTS

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
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)
    
    FIX_GFXCOORD(xMin);
    FIX_GFXCOORD(yMin);
    FIX_GFXCOORD(xMax);
    FIX_GFXCOORD(yMax);
    
    if ((xMax >= xMin) && (yMax >= yMin))
    {
	if (rp->AreaPtrn)
	{
    	    /* When rasport has areaptrn, let BltPattern do the job */
	    BltPattern(rp, NULL, xMin, yMin, xMax, yMax, 0);

	}
	else
	{
	    struct BitMap   *bm = rp->BitMap;
	    UBYTE   	    rp_drmd;

	    HIDDT_Pixel     pix;
	    HIDDT_DrawMode  drmd = 0;
	    ULONG   	    pen;

	    /* Get drawmode */
	    rp_drmd = GetDrMd(rp);

	    pen = (rp_drmd & INVERSVID ? GetBPen(rp) : GetAPen(rp));

	    /* Get rectfill pixel */

	    pix = BM_PIXEL(bm, pen);


	    if (rp_drmd & JAM2)
	    {
    		drmd = vHidd_GC_DrawMode_Copy;
	    }
	    else if (rp_drmd & COMPLEMENT)
	    {
    		drmd = vHidd_GC_DrawMode_Invert;
	    }
	    else if ((rp_drmd & (~INVERSVID)) == JAM1)
	    {
    		drmd = vHidd_GC_DrawMode_Copy;
	    }

	    fillrect_pendrmd(rp, xMin, yMin, xMax, yMax, pix, drmd, GfxBase);

	}
	
    } /* if ((xMax >= xMin) && (yMax >= yMin)) */
    
    AROS_LIBFUNC_EXIT

} /* RectFill */
