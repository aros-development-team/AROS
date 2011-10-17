/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$    $Log

    Desc: Graphics function DrawEllipse
    Lang: english
*/

#include <aros/debug.h>
#include <clib/macros.h>
#include <graphics/rastport.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"
#include "intregions.h"

/****************************************************************************************/

struct ellipse_render_data
{
    struct render_special_info rsi;
    LONG a, b;    
};

static ULONG ellipse_render(APTR ellipse_rd, LONG srcx, LONG srcy,
    	    	    	   OOP_Object *dstbm_obj, OOP_Object *dst_gc,
    	    	    	   struct Rectangle *rect, struct GfxBase *GfxBase)
{
    struct ellipse_render_data *erd = ellipse_rd;

    HIDD_GC_SetClipRect(dst_gc, rect->MinX, rect->MinY, rect->MaxX, rect->MaxY);

    HIDD_BM_DrawEllipse(dstbm_obj, dst_gc, erd->a + rect->MinX - srcx, erd->b + rect->MinY - srcy,
					   erd->a, erd->b);

    HIDD_GC_UnsetClipRect(dst_gc);

    return 0;
}

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH5(void, DrawEllipse,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(LONG             , xCenter, D0),
	AROS_LHA(LONG             , yCenter, D1),
	AROS_LHA(LONG             , a, D2),
	AROS_LHA(LONG             , b, D3),

/*  LOCATION */
	struct GfxBase *, GfxBase, 30, Graphics)

/*  FUNCTION
	Draw an ellipse

    INPUTS
	rp              - destination RastPort
	xCenter,yCenter - coordinate of centerpoint
	a               - radius in x direction
	b               - radius in y direction

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

    struct Rectangle 	rr;
    struct ellipse_render_data erd;
    
    if (!OBTAIN_DRIVERDATA(rp, GfxBase))
	return;

    FIX_GFXCOORD(xCenter);
    FIX_GFXCOORD(yCenter);
    FIX_GFXCOORD(a);
    FIX_GFXCOORD(b);
    
    /* bug("driver_DrawEllipse(%d %d %d %d)\n", xCenter, yCenter, a, b);	
    */
    rr.MinX = xCenter - a;
    rr.MinY = yCenter - b;
    rr.MaxX = xCenter + a;
    rr.MaxY = yCenter + b;

    erd.a = a;
    erd.b = b;
    
    do_render_func(rp, NULL, &rr, ellipse_render, &erd, TRUE, TRUE, GfxBase);
    
    RELEASE_DRIVERDATA(rp, GfxBase);
    
    AROS_LIBFUNC_EXIT
    
} /* DrawEllipse */
