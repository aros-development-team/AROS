/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$    $Log

    Desc: Graphics function DrawEllipse
    Lang: english
*/
#include <aros/debug.h>
#include <clib/macros.h>
#include "graphics_intern.h"
#include <graphics/rastport.h>
#include "gfxfuncsupport.h"
#include "intregions.h"

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

    struct Rectangle 	rr;
    OOP_Object      	*gc;
    struct Layer    	*L = rp->Layer;
    struct BitMap   	*bm = rp->BitMap;
    struct Rectangle 	 rp_clip_rectangle;
    BOOL    	    	 have_rp_cliprectangle;
    
    if (!OBTAIN_DRIVERDATA(rp, GfxBase))
	return;

    FIX_GFXCOORD(xCenter);
    FIX_GFXCOORD(yCenter);
    FIX_GFXCOORD(a);
    FIX_GFXCOORD(b);
    
    /* bug("driver_DrawEllipse(%d %d %d %d)\n", xCenter, yCenter, a, b);	
    */    gc = GetDriverData(rp)->dd_GC;
    
    rr.MinX = xCenter - a;
    rr.MinY = yCenter - b;
    rr.MaxX = xCenter + a;
    rr.MaxY = yCenter + b;
  
    if (NULL == L)
    {
        /* No layer, probably a screen, but may be a user inited bitmap */
	OOP_Object *bm_obj;
	
	bm_obj = OBTAIN_HIDD_BM(bm);
	if (bm_obj)
	{
	    /* No need for clipping */
	    HIDD_BM_DrawEllipse(bm_obj, gc
		    , xCenter, yCenter
		    , a, b
	    );

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
	
	CR = L->ClipRect;
	
	xrel = L->bounds.MinX;
	yrel = L->bounds.MinY;
	
	xCenter -= L->Scroll_X;
	yCenter -= L->Scroll_Y;

	have_rp_cliprectangle = GetRPClipRectangleForLayer(rp, L, &rp_clip_rectangle, GfxBase);
	
	torender.MinX = rr.MinX + xrel - L->Scroll_X;
	torender.MinY = rr.MinY + yrel - L->Scroll_Y;
	torender.MaxX = rr.MaxX + xrel - L->Scroll_X;
	torender.MaxY = rr.MaxY + yrel - L->Scroll_Y;
		
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
    	    		/* bug("Setting cliprect: %d %d %d %d : layerrel: %d %d %d %d\n"
		    	    , intersect.MinX
			    , intersect.MinY
			    , intersect.MaxX
			    , intersect.MaxY

		    	    , intersect.MinX - xrel
			    , intersect.MinY - yrel
			    , intersect.MaxX - xrel
			    , intersect.MaxY - yrel
			);
    	    		*/		    
			HIDD_GC_SetClipRect(gc
		    	    , intersect.MinX
			    , intersect.MinY
			    , intersect.MaxX
			    , intersect.MaxY
			);

			HIDD_BM_DrawEllipse(HIDD_BM_OBJ(bm)
		    	    , gc
			    , xCenter + xrel
			    , yCenter + yrel
			    , a
			    , b
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

			    HIDD_BM_DrawEllipse(HIDD_BM_OBJ(CR->BitMap)
				    , gc
				    , bm_rel_minx - (layer_rel_x - xCenter) + ALIGN_OFFSET(CR->bounds.MinX)
				    , bm_rel_miny - (layer_rel_y - yCenter)
				    , a
				    , b
			    );


			    HIDD_GC_UnsetClipRect(gc);
			}

		    } /* if (CR->lobs == NULL) */
		
		} /* if it also intersects with possible rastport clip rectangle */

	    } /* if (cliprect intersects with area to render into) */
	    
	} /* for (each cliprect in the layer) */
	
        UnlockLayerRom(L);
	
    } /* if (rp->Layer) */
    
    RELEASE_DRIVERDATA(rp, GfxBase);
    
    AROS_LIBFUNC_EXIT
    
} /* DrawEllipse */
