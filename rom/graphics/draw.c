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
    struct Rectangle 	 rp_clip_rectangle;
    BOOL    	    	 have_rp_cliprectangle;
    LONG    	    	 dx, dy;
    LONG    	    	 x1, y1;
    
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
         
	
    if (NULL == L)
    {
        /* No layer, probably a screen, but may be a user inited bitmap */
	OOP_Object *bm_obj;

	bm_obj = OBTAIN_HIDD_BM(bm);
	if (bm_obj)
	{
	    /* No need for clipping */
	    HIDD_BM_DrawLine(bm_obj, gc, x1, y1, x, y);  
	
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

    	x1 -= L->Scroll_X;
	y1 -= L->Scroll_Y;
	x  -= L->Scroll_X;
	y  -= L->Scroll_Y;
	
	have_rp_cliprectangle = GetRPClipRectangleForLayer(rp, L, &rp_clip_rectangle, GfxBase);
	
	xrel = L->bounds.MinX;
	yrel = L->bounds.MinY;
		
	torender.MinX = rr.MinX + xrel - L->Scroll_X;
	torender.MinY = rr.MinY + yrel - L->Scroll_Y;
	torender.MaxX = rr.MaxX + xrel - L->Scroll_X;
	torender.MaxY = rr.MaxY + yrel - L->Scroll_Y;
		
	CR = L->ClipRect;
	
	for (;NULL != CR; CR = CR->Next)
	{
	    D(bug("Cliprect (%d, %d, %d, %d), lobs=%p\n",
	    	CR->bounds.MinX, CR->bounds.MinY, CR->bounds.MaxX, CR->bounds.MaxY,
		CR->lobs));
		
	    /* Does this cliprect intersect with area to rectfill ? */
	    if (_AndRectRect(&CR->bounds, &torender, &intersect))
	    {
	    	if (!have_rp_cliprectangle || _AndRectRect(&rp_clip_rectangle, &intersect, &intersect))
		{
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
			    , x1 + xrel
			    , y1 + yrel
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
				    , bm_rel_minx - (layer_rel_x - x1) + ALIGN_OFFSET(CR->bounds.MinX)
				    , bm_rel_miny - (layer_rel_y - y1)
				    , bm_rel_minx - (layer_rel_x - x) + ALIGN_OFFSET(CR->bounds.MinX)
				    , bm_rel_miny - (layer_rel_y - y)
			    );

			    HIDD_GC_UnsetClipRect(gc);
			}

		    } /* if (CR->lobs == NULL) */
		
		} /* if it also intersects with possible rastport clip rectangle */
		
	    } /* if (cliprect intersects with area to render into) */
	    
	} /* for (each cliprect in the layer) */
	
        UnlockLayerRom(L);
	
    } /* if (rp->Layer) */
    
    dx = abs(x1 - x);
    dy = abs(y1 - y);
    if (dy > dx) dx = dy;
    
    rp->linpatcnt = ((LONG)rp->linpatcnt - dx) & 15;

    rp->cp_x = x;
    rp->cp_y = y;
    
    RELEASE_DRIVERDATA(rp, GfxBase);
    
    AROS_LIBFUNC_EXIT
    
} /* Draw */
