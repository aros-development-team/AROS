/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/* !!!! ONLY USE THE BELOW MACROS IF YOU ARE 100% SURE 
   THAT IT IS A HIDD BITMAP AND NOT ONE THE USER
   HAS CREATED BY HAND !!!. You can use IS_HIDD_BM(bitmap) to test
   if it is a HIDD bitmap
*/

/****************************************************************************************/

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/cybergraphics.h>
#include <proto/oop.h>
#include <clib/macros.h>

#include "graphics_intern.h"
#include "objcache.h"
#include "intregions.h"
#include "gfxfuncsupport.h"

#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/

OOP_Object *get_planarbm_object(struct BitMap *bitmap, struct GfxBase *GfxBase)
{
    OOP_Object *pbm_obj;

    D(bug("get_planarbm_object()\n"));    
    pbm_obj = obtain_cache_object(SDD(GfxBase)->planarbm_cache, GfxBase);
    
    if (NULL != pbm_obj)
    {
	
	D(bug("Got cache object %p, class=%s, domethod=%p, instoffset=%d\n"
		, pbm_obj
		, OOP_OCLASS(pbm_obj)->ClassNode.ln_Name
		, OOP_OCLASS(pbm_obj)->DoMethod
		, OOP_OCLASS(pbm_obj)->InstOffset
	));
	
    	if (!HIDD_PlanarBM_SetBitMap(pbm_obj, bitmap))
	{
	     D(bug("!!! get_planarbm_object: HIDD_PlanarBM_SetBitMap FAILED !!!\n"));
	     release_cache_object(SDD(GfxBase)->planarbm_cache, pbm_obj, GfxBase);
	     pbm_obj = NULL;
	}
		
    }
    else
    {
    	D(bug("!!! get_planarbm_object: obtain_cache_object FAILED !!!\n"));
    }
    
    return pbm_obj;
}

/****************************************************************************************/

ULONG do_render_func(struct RastPort *rp
	, Point *src
	, struct Rectangle *rr
	, ULONG (*render_func)(APTR, LONG, LONG, OOP_Object *, OOP_Object *, LONG, LONG, LONG, LONG, struct GfxBase *)
	, APTR funcdata
	, BOOL get_special_info
	, struct GfxBase *GfxBase)
{

    struct BitMap *bm = rp->BitMap;
    struct Layer *L = rp->Layer;
    OOP_Object *gc;
    ULONG width, height;
    LONG srcx, srcy;
    
    LONG pixwritten = 0;
    
    gc = GetDriverData(rp)->dd_GC;
	
    width  = rr->MaxX - rr->MinX + 1;
    height = rr->MaxY - rr->MinY + 1;
    
    if (NULL != src)
    {
        srcx = src->x;
	srcy = src->y;
    } else
    {
    	srcx = 0;
	srcy = 0;
    }
    
    if (NULL == L)
    {
        /* No layer, probably a screen, but may be a user inited bitmap */
	OOP_Object *bm_obj;
	
	bm_obj = OBTAIN_HIDD_BM(bm);
	if (NULL == bm_obj)
	    return 0;
	    
	if (get_special_info)
	{
	    RSI(funcdata)->curbm    = rp->BitMap;
	    RSI(funcdata)->onscreen = TRUE;
	    RSI(funcdata)->layer_rel_srcx = srcx;
	    RSI(funcdata)->layer_rel_srcy = srcy;
	}
	    
	pixwritten = render_func(funcdata
		, srcx, srcy
		, bm_obj, gc
		, rr->MinX, rr->MinY
		, rr->MaxX, rr->MaxY
		, GfxBase
	);

	RELEASE_HIDD_BM(bm_obj, bm);

    }
    else
    {
        struct ClipRect *CR;
	WORD xrel;
        WORD yrel;
	struct Rectangle torender, intersect;
	
	LockLayerRom(L);
	
	xrel = L->bounds.MinX;
	yrel = L->bounds.MinY;

	torender.MinX = rr->MinX + xrel;
	torender.MinY = rr->MinY + yrel;
	torender.MaxX = rr->MaxX + xrel;
	torender.MaxY = rr->MaxY + yrel;
	
	
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
		
		xoffset = intersect.MinX - torender.MinX;
		yoffset = intersect.MinY - torender.MinY;
		
		if (get_special_info) {
		     RSI(funcdata)->layer_rel_srcx = intersect.MinX - L->bounds.MinX;
		     RSI(funcdata)->layer_rel_srcy = intersect.MinY - L->bounds.MinY;
		}
		
	        if (NULL == CR->lobs)
		{
		    if (get_special_info)
		    {
			RSI(funcdata)->curbm = bm;
			RSI(funcdata)->onscreen = TRUE;
		    }
		    
		    pixwritten += render_func(funcdata
		    	, srcx + xoffset
			, srcy + yoffset
		        , HIDD_BM_OBJ(bm)
		    	, gc
		    	, intersect.MinX
			, intersect.MinY
			, intersect.MaxX
			, intersect.MaxY
			, GfxBase
		    );
		
		
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

		    	if (get_special_info)
			{
			    RSI(funcdata)->curbm = CR->BitMap;
			    RSI(funcdata)->onscreen = FALSE;
		    	}
			pixwritten += render_func(funcdata
				, srcx + xoffset, srcy + yoffset
		        	, HIDD_BM_OBJ(CR->BitMap)
		    		, gc
		    		, intersect.MinX - CR->bounds.MinX + ALIGN_OFFSET(CR->bounds.MinX)
				, intersect.MinY - CR->bounds.MinY
				, intersect.MaxX - CR->bounds.MinX + ALIGN_OFFSET(CR->bounds.MinX) 
				, intersect.MaxY - CR->bounds.MinY
				, GfxBase
		    	);
		    }
		    
		} /* if (CR->lobs == NULL) */
		
	    } /* if (cliprect intersects with area to render into) */
	    
	} /* for (each cliprect in the layer) */
	
        UnlockLayerRom(L);
    } /* if (rp->Layer) */
    
	
    return pixwritten;

}

/****************************************************************************************/

ULONG do_pixel_func(struct RastPort *rp
	, LONG x, LONG y
	, LONG (*render_func)(APTR, OOP_Object *, OOP_Object *, LONG, LONG, struct GfxBase *)
	, APTR funcdata
	, struct GfxBase *GfxBase)
{
    struct BitMap *bm = rp->BitMap;
    struct Layer *L = rp->Layer;
    OOP_Object *gc;
    ULONG retval = -1;
   
    gc = GetDriverData(rp)->dd_GC;
   
    if (NULL == L)
    {
	OOP_Object *bm_obj;
	ULONG width, height;
	
	bm_obj = OBTAIN_HIDD_BM(bm);
	if (NULL == bm_obj)
	    return -1;
	
	OOP_GetAttr(bm_obj, aHidd_BitMap_Width,  &width);
	OOP_GetAttr(bm_obj, aHidd_BitMap_Height, &height);

	/* Check whether we it is inside the rastport */
	if (	x <  0
	     || x >= width
	     || y <  0
	     || y >= height)
	{
	     
	     RELEASE_HIDD_BM(bm_obj, bm);
	     return -1;
	     
	}
	
    	/* This is a screen */
	retval = render_func(funcdata, bm_obj, gc, x, y, GfxBase);
	
	RELEASE_HIDD_BM(bm_obj, bm);
	
    }
    else
    {
        struct ClipRect *CR;
	LONG absx, absy;
	
	LockLayerRom( L );
	
	CR = L->ClipRect;
	
	absx = x + L->bounds.MinX;
	absy = y + L->bounds.MinY;
	
	for (;NULL != CR; CR = CR->Next)
	{
	
	    if (    absx >= CR->bounds.MinX
	         && absy >= CR->bounds.MinY
		 && absx <= CR->bounds.MaxX
		 && absy <= CR->bounds.MaxY )
	    {


	
	        if (NULL == CR->lobs)
		{
		    retval = render_func(funcdata
		    	, HIDD_BM_OBJ(bm), gc
			, absx, absy
			, GfxBase
		    );
		}
		else 
		{
		    /* This is the tricky one: render into offscreen cliprect bitmap */
		    if (L->Flags & LAYERSIMPLE)
		    {
		    	/* We cannot do anything */
		    	retval =  0;
		
		    }
		    else if (L->Flags & LAYERSUPER)
		    {
		    	D(bug("driver_WriteRGBPixel(): Superbitmap not handled yet\n"));
		    }
		    else
		    {
			retval = render_func(funcdata
				, HIDD_BM_OBJ(CR->BitMap), gc
				, absx - CR->bounds.MinX + ALIGN_OFFSET(CR->bounds.MinX)
				, absy - CR->bounds.MinY
				, GfxBase
			); 


		    } /* If (SMARTREFRESH cliprect) */
		    
		    
		}   /* if (intersecton inside hidden cliprect) */
		
		/* The pixel was found and put inside one of the cliprects, just exit */
		break;

	    } /* if (cliprect intersects with area we want to draw to) */
	    
	} /* while (cliprects to examine) */
	
	UnlockLayerRom( L );
    
    }
    
    return retval;

}

/****************************************************************************************/

static ULONG fillrect_render(APTR funcdata, LONG srcx, LONG srcy,
    	    	    	     OOP_Object *dstbm_obj, OOP_Object *dst_gc,
			     LONG x1, LONG y1, LONG x2, LONG y2,
			     struct GfxBase *GfxBase)
{

    HIDD_BM_FillRect(dstbm_obj, dst_gc, x1, y1, x2, y2);
    
    return (x2 - x1 + 1) * (y2 - y1 + 1);
}

/****************************************************************************************/

LONG fillrect_pendrmd(struct RastPort *rp, LONG x1, LONG y1, LONG x2, LONG y2,
    	    	      HIDDT_Pixel pix, HIDDT_DrawMode drmd, struct GfxBase *GfxBase)
{   
    LONG    	    	pixwritten = 0;
    
    HIDDT_DrawMode  	old_drmd;
    HIDDT_Pixel     	old_fg;
    OOP_Object      	*gc;
    struct Rectangle 	rr;

    struct TagItem gc_tags[] =
    {
	{ aHidd_GC_DrawMode 	, drmd  },
	{ aHidd_GC_Foreground	, pix 	},
	{ TAG_DONE  	    	    	}
    };

    
    if (!CorrectDriverData (rp, GfxBase))
	return 0;
	
    gc = GetDriverData(rp)->dd_GC;
	
    OOP_GetAttr(gc, aHidd_GC_DrawMode,	(IPTR *)&old_drmd);
    OOP_GetAttr(gc, aHidd_GC_Foreground,(IPTR *)&old_fg);
    
    OOP_SetAttrs(gc, gc_tags);
    
    rr.MinX = x1;
    rr.MinY = y1;
    rr.MaxX = x2;
    rr.MaxY = y2;
    
    pixwritten = do_render_func(rp, NULL, &rr, fillrect_render, NULL, FALSE, GfxBase);
    
    /* Restore old GC values */
    gc_tags[0].ti_Data = (IPTR)old_drmd;
    gc_tags[1].ti_Data = (IPTR)old_fg;
    OOP_SetAttrs(gc, gc_tags);
	
    return pixwritten;
}

/****************************************************************************************/

BOOL int_bltbitmap(struct BitMap *srcBitMap, OOP_Object *srcbm_obj, LONG xSrc, LONG ySrc,
	    	   struct BitMap *dstBitMap, OOP_Object *dstbm_obj, LONG xDest, LONG yDest,
		   LONG xSize, LONG ySize, ULONG minterm, OOP_Object *gc, struct GfxBase *GfxBase)
{
    HIDDT_DrawMode drmd;

    ULONG srcflags = 0;
    ULONG dstflags = 0;

    BOOL src_colmap_set = FALSE;
    BOOL dst_colmap_set = FALSE;
    BOOL success = TRUE;
    BOOL colmaps_ok = TRUE;

    drmd = MINTERM_TO_GCDRMD(minterm);
    
    /* We must lock any HIDD_BM_SetColorMap calls */
    LOCK_BLIT

    /* Try to get a CLUT for the bitmaps */
    if (IS_HIDD_BM(srcBitMap))
    {
    	//bug("driver_intbltbitmap: source is hidd bitmap\n");
    	if (NULL != HIDD_BM_COLMAP(srcBitMap))
    	{
    	    //bug("driver_intbltbitmap: source has colormap\n");
    	    srcflags |= FLG_HASCOLMAP;
    	}
    	srcflags |= GET_COLMOD_FLAGS(srcBitMap);
    }
    else
    {
    	//bug("driver_intbltbitmap: source is amiga bitmap\n");
    	/* Amiga BM */
    	srcflags |= FLG_PALETTE;
    }

    if (IS_HIDD_BM(dstBitMap))
    {
    	//bug("driver_intbltbitmap: dest is hidd bitmap\n");
    	if (NULL != HIDD_BM_COLMAP(dstBitMap))
    	{
    	    //bug("driver_intbltbitmap: dest has colormap\n");
    	    dstflags |= FLG_HASCOLMAP;
    	}
    	dstflags |= GET_COLMOD_FLAGS(dstBitMap);
    }
    else
    {
    	//bug("driver_intbltbitmap: dest is amiga bitmap\n");
    	/* Amiga BM */
    	dstflags |= FLG_PALETTE;
    }
    	
    if (    (srcflags == FLG_PALETTE || srcflags == FLG_STATICPALETTE))
    {
    	/* palettized with no colmap. Neew to get a colmap from dest*/
    	if (dstflags == FLG_TRUECOLOR)
	{
    	
    	    D(bug("!!! NO WAY GETTING PALETTE FOR src IN BltBitMap\n"));
    	    colmaps_ok = FALSE;
	    success = FALSE;
    	    
    	}
	else if (dstflags == (FLG_TRUECOLOR | FLG_HASCOLMAP))
	{
    	
    	    /* Use the dest colmap for src */
    	    HIDD_BM_SetColorMap(srcbm_obj, HIDD_BM_COLMAP(dstBitMap));

	    src_colmap_set = TRUE;

	    /* 		
	    bug("Colormap:\n");
	    {
	    ULONG idx;
	    for (idx = 0; idx < 256; idx ++)
		    bug("[%d]=%d ", idx, HIDD_CM_GetPixel(HIDD_BM_COLMAP(dstBitMap), idx));
	    }
	    */
	}
    }

    if (   (dstflags == FLG_PALETTE || dstflags == FLG_STATICPALETTE))
    {
    	/* palettized with no pixtab. Nees to get a pixtab from dest*/
    	if (srcflags == FLG_TRUECOLOR)
	{
    	    D(bug("!!! NO WAY GETTING PALETTE FOR dst IN BltBitMap\n"));
    	    colmaps_ok = FALSE;
	    success = FALSE;
    	    
    	}
	else if (srcflags == (FLG_TRUECOLOR | FLG_HASCOLMAP))
	{
    	
    	    /* Use the src colmap for dst */
    	    HIDD_BM_SetColorMap(dstbm_obj, HIDD_BM_COLMAP(srcBitMap));
    	    
    	    dst_colmap_set = TRUE;
    	}
    }
    	    
    if (colmaps_ok)
    {
    	/* We need special treatment with drawmode Clear and
    	   truecolor bitmaps, in order to set it to
    	   colormap[0] instead of just 0
    	*/
    	if (	(drmd == vHidd_GC_DrawMode_Clear)
    	     && ( (dstflags & (FLG_TRUECOLOR | FLG_HASCOLMAP)) == (FLG_TRUECOLOR | FLG_HASCOLMAP) ))
	{
    	     
	    HIDDT_DrawMode old_drmd;
	    HIDDT_Pixel old_fg;
	    
    	    struct TagItem frtags[] =
	    {
    		 { aHidd_GC_Foreground	, 0 	    	    	    },
    		 { aHidd_GC_DrawMode	, vHidd_GC_DrawMode_Copy    },
    		 { TAG_DONE 	    	    	    	    	    }
    	    };
	    
	    OOP_GetAttr(gc, aHidd_GC_DrawMode, &old_drmd);
	    OOP_GetAttr(gc, aHidd_GC_Foreground, &old_fg);
    	    
    	    frtags[0].ti_Data = HIDD_BM_PIXTAB(dstBitMap)[0];
	    frtags[1].ti_Data = vHidd_GC_DrawMode_Copy;
	    
    	    OOP_SetAttrs(gc, frtags);
    	    
    	    HIDD_BM_FillRect(dstbm_obj, gc
    		    , xDest, yDest
    		    , xDest + xSize - 1
    		    , yDest + ySize - 1
    	    );

    	    frtags[0].ti_Data = old_fg;
	    frtags[1].ti_Data = old_drmd;
    	
    	}
	else
	{
	    HIDDT_DrawMode old_drmd;
	    
	    struct TagItem cbtags[] =
	    {
    		{ aHidd_GC_DrawMode, 	    0 },
    		{ TAG_DONE  	    	      }
	    };
	    
	    OOP_GetAttr(gc, aHidd_GC_DrawMode, &old_drmd);
	    
	    cbtags[0].ti_Data = drmd;
	    
	    OOP_SetAttrs(gc, cbtags);
    	    HIDD_Gfx_CopyBox(SDD(GfxBase)->gfxhidd
	    	, srcbm_obj
    		, xSrc, ySrc
    		, dstbm_obj
    		, xDest, yDest
    		, xSize, ySize
		, gc
    	    );
	    
	    cbtags[0].ti_Data = drmd;
	    OOP_SetAttrs(gc, cbtags);
    	}
	
    } /* if (colmaps_ok) */

    if (src_colmap_set)
    	HIDD_BM_SetColorMap(srcbm_obj, NULL);
    	
    if (dst_colmap_set)
    	HIDD_BM_SetColorMap(dstbm_obj, NULL);
	
    ULOCK_BLIT
	
    return success;

}

/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
