/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

    INPUTS

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
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)


    struct Rectangle 	rr;
    OOP_Object      	*gc;
    struct Layer    	*L = rp->Layer;
    struct BitMap   	*bm = rp->BitMap;
    LONG    	    	 dx, dy;
    
    if (!OBTAIN_DRIVERDATA(rp, GfxBase))
	return;

    FIX_GFXCOORD(x);
    FIX_GFXCOORD(y);
    
    gc = GetDriverData(rp)->dd_GC;

    if (rp->cp_x > x)
    {
	rr.MinX = x;
	rr.MaxX = rp->cp_x;
    }
    else
    {
    	rr.MinX = rp->cp_x;
	rr.MaxX = x;
    }
    
    if (rp->cp_y > y)
    {
	rr.MinY = y;
	rr.MaxY = rp->cp_y;
    }
    else
    {
    	rr.MinY = rp->cp_y;
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
         
	
    if (NULL == L)
    {
        /* No layer, probably a screen, but may be a user inited bitmap */
	OOP_Object *bm_obj;

	bm_obj = OBTAIN_HIDD_BM(bm);
	if (bm_obj)
	{
	    /* No need for clipping */
	    HIDD_BM_DrawLine(bm_obj, gc, rp->cp_x, rp->cp_y, x, y);  
	
	    RELEASE_HIDD_BM(bm_obj, bm);
	}
	    
    }
    else
    {
        struct ClipRect     *CR;
	WORD 	    	    xrel;
        WORD 	    	    yrel;
	struct Rectangle    torender, intersect;

	LockLayerRom(L);
	
	xrel = L->bounds.MinX;
	yrel = L->bounds.MinY;
		
	torender.MinX = rr.MinX + xrel;
	torender.MinY = rr.MinY + yrel;
	torender.MaxX = rr.MaxX + xrel;
	torender.MaxY = rr.MaxY + yrel;
		
	CR = L->ClipRect;
	
	for (;NULL != CR; CR = CR->Next)
	{
	    D(bug("Cliprect (%d, %d, %d, %d), lobs=%p\n",
	    	CR->bounds.MinX, CR->bounds.MinY, CR->bounds.MaxX, CR->bounds.MaxY,
		CR->lobs));
		
	    /* Does this cliprect intersect with area to rectfill ? */
	    if (_AndRectRect(&CR->bounds, &torender, &intersect))
	    {
	    	LONG xoffset, yoffset;
		LONG layer_rel_x, layer_rel_y;
		
		xoffset = intersect.MinX - torender.MinX;
		yoffset = intersect.MinY - torender.MinY;
		
		layer_rel_x = intersect.MinX - L->bounds.MinX;
		layer_rel_y = intersect.MinY - L->bounds.MinY;
					
	        if (NULL == CR->lobs)
		{		
		    /* Set clip rectangle */
		    HIDD_GC_SetClipRect(gc
		    	, intersect.MinX
			, intersect.MinY
			, intersect.MaxX
			, intersect.MaxY
		    );
		    
		    HIDD_BM_DrawLine(HIDD_BM_OBJ(bm)
		    	, gc
			, rp->cp_x + xrel
			, rp->cp_y + yrel
			, x + xrel
			, y + yrel
		    );
		    
		    HIDD_GC_UnsetClipRect(gc);
		
		}
		else
		{
		    /* Render into offscreen cliprect bitmap */
		    if (L->Flags & LAYERSIMPLE)
		    	continue;
		    else if (L->Flags & LAYERSUPER)
		    {
		    	D(bug("do_render_func(): Superbitmap not handled yet\n"));
		    }
		    else
		    {
		    	LONG bm_rel_minx, bm_rel_miny, bm_rel_maxx, bm_rel_maxy;
			LONG layer_rel_x, layer_rel_y;

			layer_rel_x = intersect.MinX - xrel;
			layer_rel_y = intersect.MinY - yrel;
			
			bm_rel_minx = intersect.MinX - CR->bounds.MinX;
			bm_rel_miny = intersect.MinY - CR->bounds.MinY;
			bm_rel_maxx = intersect.MaxX - CR->bounds.MinX;
			bm_rel_maxy = intersect.MaxY - CR->bounds.MinY;

		    	HIDD_GC_SetClipRect(gc
		    		, bm_rel_minx + ALIGN_OFFSET(CR->bounds.MinX)
				, bm_rel_miny
				, bm_rel_maxx + ALIGN_OFFSET(CR->bounds.MinX) 
				, bm_rel_maxy
			);
			
			HIDD_BM_DrawLine(HIDD_BM_OBJ(CR->BitMap)
				, gc
				, bm_rel_minx - (layer_rel_x - rp->cp_x) + ALIGN_OFFSET(CR->bounds.MinX)
				, bm_rel_miny - (layer_rel_y - rp->cp_y)
				, bm_rel_minx - (layer_rel_x - x) + ALIGN_OFFSET(CR->bounds.MinX)
				, bm_rel_miny - (layer_rel_y - y)
			);
				
			HIDD_GC_UnsetClipRect(gc);
		    }
		    
		} /* if (CR->lobs == NULL) */
		
	    } /* if (cliprect intersects with area to render into) */
	    
	} /* for (each cliprect in the layer) */
	
        UnlockLayerRom(L);
	
    } /* if (rp->Layer) */
    
    dx = abs(rp->cp_x - x);
    dy = abs(rp->cp_y - y);
    if (dy > dx) dx = dy;
    
    rp->linpatcnt = ((LONG)rp->linpatcnt - dx) & 15;

    rp->cp_x = x;
    rp->cp_y = y;
    
    RELEASE_DRIVERDATA(rp, GfxBase);
    
    AROS_LIBFUNC_EXIT
    
} /* Draw */
