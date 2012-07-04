/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function Draw()
    Lang: english
*/

#include <aros/debug.h>
#include <clib/macros.h>
#include <graphics/rastport.h>
#include <proto/graphics.h>

#include "gfxfuncsupport.h"
#include "graphics_intern.h"
#include "graphics_driver.h"
#include "intregions.h"

/****************************************************************************************/

struct draw_render_data
{
    WORD x1, y1, x2, y2;    
};

static ULONG draw_render(APTR draw_rd, WORD srcx, WORD srcy,
    	    	    	   OOP_Object *dstbm_obj, OOP_Object *dst_gc,
    	    	    	   struct Rectangle *rect, struct GfxBase *GfxBase)
{
    struct draw_render_data *drd = draw_rd;

    /*
     * This is a quick way to install ClipRect on a GC. Just set one pointer.
     * This is why we have a struct Rectangle * in the GC.
     */
    GC_DOCLIP(dst_gc) = rect;

    HIDD_BM_DrawLine(dstbm_obj, dst_gc, drd->x1 + rect->MinX - srcx,
    	    	    	    	    	drd->y1 + rect->MinY - srcy,
					drd->x2 + rect->MinX - srcx,
					drd->y2 + rect->MinY - srcy); 

    /*
     * After we exit this routine, 'rect' will be not valid any more.
     * So do not forget to reset the pointer!
     */
    GC_DOCLIP(dst_gc) = NULL;

    return 0;
}

/*****************************************************************************

    NAME */

	AROS_LH3(void, Draw,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(WORD             , x, D0),
	AROS_LHA(WORD             , y, D1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 41, Graphics)

/*  FUNCTION
	Draw a line from the current pen position to the given coordinate.

    INPUTS
    	rp  - destination RastPort.
	x,y - line end coordinate.

    RESULT

    NOTES
    	Not yet implemented:
	
	  - handle layer->Scroll_X/Scroll_Y.
	  
	  - handle FRST_DOT which indicates whether to draw
	    or to don't draw first pixel of line. Important
	    for COMPLEMENT drawmode.
	
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

    struct Rectangle 	    rr;
    OOP_Object      	    *gc;
    struct draw_render_data drd;
    WORD    	    	    dx;
    WORD    	    	    x1, y1;

    FIX_GFXCOORD(x);
    FIX_GFXCOORD(y);
    
    x1 = rp->cp_x;
    y1 = rp->cp_y;

    if (x1 > x)
    {
	rr.MinX = x;
	rr.MaxX = x1;
    }
    else
    {
    	rr.MinX = x1;
	rr.MaxX = x;
    }
    
    if (y1 > y)
    {
	rr.MinY = y;
	rr.MaxY = y1;
    }
    else
    {
    	rr.MinY = y1;
	rr.MaxY = y;
    }

    gc = GetDriverData(rp, GfxBase);

    /* Only Draw() uses line pattern attributes, so we set them only here */
    GC_LINEPAT(gc)    = (rp->DrawMode & INVERSVID) ? ~rp->LinePtrn : rp->LinePtrn;
    GC_LINEPATCNT(gc) = rp->linpatcnt;

    drd.x1 = x1 - rr.MinX;
    drd.y1 = y1 - rr.MinY;
    drd.x2 = x  - rr.MinX;
    drd.y2 = y  - rr.MinY;

    D(bug("[Draw] (%d, %d) to (%d, %d)\n", rp->cp_x, rp->cp_y, x, y));
    D(bug("[Draw] RastPort 0x%p, Flags 0x%04X, GC 0x%p, FG 0x%08lX, BG 0x%08lX\n", rp, rp->Flags, gc, GC_FG(gc), GC_BG(gc)));

    do_render_with_gc(rp, NULL, &rr, draw_render, &drd, gc, TRUE, FALSE, GfxBase);

    dx = (drd.x2 > drd.y2) ? drd.x2 : drd.y2;

    rp->linpatcnt = ((LONG)rp->linpatcnt - dx) & 15;

    rp->cp_x = x;
    rp->cp_y = y;

    AROS_LIBFUNC_EXIT
    
} /* Draw */
