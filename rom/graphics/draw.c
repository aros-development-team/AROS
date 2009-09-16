/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function Draw()
    Lang: english
*/

#include <aros/debug.h>
#include <clib/macros.h>
#include <graphics/rastport.h>
#include <proto/graphics.h>
#include <proto/oop.h>
#include "gfxfuncsupport.h"
#include "graphics_intern.h"
#include "intregions.h"
#include <stdlib.h>

struct draw_render_data
{
    struct render_special_info rsi;
    LONG x1, y1, x2, y2;    
};

static ULONG draw_render(APTR draw_rd, LONG srcx, LONG srcy,
    	    	    	   OOP_Object *dstbm_obj, OOP_Object *dst_gc,
			   LONG x1, LONG y1, LONG x2, LONG y2, struct GfxBase *GfxBase);

/*****************************************************************************

    NAME */

	AROS_LH3(void, Draw,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(LONG             , x, D0),
	AROS_LHA(LONG             , y, D1),

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
    LONG    	    	    dx;
    LONG    	    	    x1, y1;
    
    if (!OBTAIN_DRIVERDATA(rp, GfxBase))
	return;

    FIX_GFXCOORD(x);
    FIX_GFXCOORD(y);
    
    x1 = rp->cp_x;
    y1 = rp->cp_y;
    
    gc = GetDriverData(rp)->dd_GC;

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

    {
    	UWORD lineptrn = rp->LinePtrn;
	
	if (rp->DrawMode & INVERSVID) lineptrn = ~lineptrn;
	
	{
    	    struct TagItem gctags[] =
	    {
		{aHidd_GC_LinePattern   , lineptrn      },
		{aHidd_GC_LinePatternCnt, rp->linpatcnt },
		{TAG_DONE	    	    	    	}
	    };

	    OOP_SetAttrs( gc, gctags);
    	}
    }
         
    drd.x1 = x1 - rr.MinX;
    drd.y1 = y1 - rr.MinY;
    drd.x2 = x - rr.MinX;
    drd.y2 = y - rr.MinY;
    
    do_render_func(rp, NULL, &rr, draw_render, &drd, TRUE, TRUE, GfxBase);
        
    dx = (drd.x2 > drd.y2) ? drd.x2 : drd.y2;
    
    rp->linpatcnt = ((LONG)rp->linpatcnt - dx) & 15;

    rp->cp_x = x;
    rp->cp_y = y;
    
    RELEASE_DRIVERDATA(rp, GfxBase);
    
    AROS_LIBFUNC_EXIT
    
} /* Draw */

/****************************************************************************************/

static ULONG draw_render(APTR draw_rd, LONG srcx, LONG srcy,
    	    	    	   OOP_Object *dstbm_obj, OOP_Object *dst_gc,
			   LONG x1, LONG y1, LONG x2, LONG y2, struct GfxBase *GfxBase)
{
    struct draw_render_data *drd;
    ULONG   	    	     width, height;

    width  = x2 - x1 + 1;
    height = y2 - y1 + 1;

    drd = (struct draw_render_data *)draw_rd;

    HIDD_GC_SetClipRect(dst_gc, x1, y1, x2, y2);

    HIDD_BM_DrawLine(dstbm_obj, dst_gc, drd->x1 + x1 - srcx,
    	    	    	    	    	drd->y1 + y1 - srcy,
					drd->x2 + x1 - srcx,
					drd->y2 + y1 - srcy); 

    HIDD_GC_UnsetClipRect(dst_gc);
   
    return 0;
}

/****************************************************************************************/
