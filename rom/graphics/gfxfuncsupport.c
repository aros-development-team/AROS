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

#include <cybergraphx/cybergraphics.h>
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

	torender.MinX = rr->MinX + xrel - L->Scroll_X;
	torender.MinY = rr->MinY + yrel - L->Scroll_Y;
	torender.MaxX = rr->MaxX + xrel - L->Scroll_X;
	torender.MaxY = rr->MaxY + yrel - L->Scroll_Y;

	
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
		     RSI(funcdata)->layer_rel_srcx = intersect.MinX - L->bounds.MinX + L->Scroll_X;
		     RSI(funcdata)->layer_rel_srcy = intersect.MinY - L->bounds.MinY + L->Scroll_Y;
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
	
	absx = x + L->bounds.MinX - L->Scroll_X;
	absy = y + L->bounds.MinY - L->Scroll_Y;
	
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


    if (!OBTAIN_DRIVERDATA(rp, GfxBase))
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
	
    RELEASE_DRIVERDATA(rp, GfxBase);
    
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

struct wp8_render_data
{
    UBYTE   	   *array;
    ULONG   	    modulo;
    HIDDT_PixelLUT *pixlut;
};

static ULONG wp8_render(APTR wp8r_data, LONG srcx, LONG srcy, OOP_Object *dstbm_obj,
    	    	    	OOP_Object *dst_gc, LONG x1, LONG y1, LONG x2, LONG y2,
			struct GfxBase *GfxBase)
{
    struct wp8_render_data *wp8rd;
    ULONG   	    	    width, height;

    wp8rd = (struct wp8_render_data *)wp8r_data;
    
    width  = x2 - x1 + 1;
    height = y2 - y1 + 1;
    
    HIDD_BM_PutImageLUT(dstbm_obj
    	, dst_gc
	, wp8rd->array + CHUNKY8_COORD_TO_BYTEIDX(srcx, srcy, wp8rd->modulo)
	, wp8rd->modulo
	, x1, y1
	, width, height
	, wp8rd->pixlut
    );
    
    return width * height;
}
/****************************************************************************************/

LONG write_pixels_8(struct RastPort *rp, UBYTE *array, ULONG modulo,
    	    	    LONG xstart, LONG ystart, LONG xstop, LONG ystop,
		    HIDDT_PixelLUT *pixlut, struct GfxBase *GfxBase)
{
	
    LONG pixwritten = 0;
    
    struct wp8_render_data wp8rd;
    struct Rectangle rr;
    
    OOP_Object *gc;
    HIDDT_DrawMode old_drmd;

    struct TagItem gc_tags[] =
    {
	{ aHidd_GC_DrawMode, vHidd_GC_DrawMode_Copy},
	{ TAG_DONE, 0}
    };
    

    if (!OBTAIN_DRIVERDATA(rp, GfxBase))
	return 0;
	
    gc = GetDriverData(rp)->dd_GC;
    
    OOP_GetAttr(gc, aHidd_GC_DrawMode, &old_drmd);
    OOP_SetAttrs(gc, gc_tags);
    
    wp8rd.modulo	= modulo;
    wp8rd.array		= array;
    wp8rd.pixlut	= pixlut;
    
    rr.MinX = xstart;
    rr.MinY = ystart;
    rr.MaxX = xstop;
    rr.MaxY = ystop;
    
    pixwritten = do_render_func(rp, NULL, &rr, wp8_render, &wp8rd, FALSE, GfxBase);
    
    /* Reset to preserved drawmode */
    gc_tags[0].ti_Data = old_drmd;
    OOP_SetAttrs(gc, gc_tags);
    
    RELEASE_DRIVERDATA(rp, GfxBase);
    
    return pixwritten;

}

/****************************************************************************************/

/*
** General functions for moving blocks of data to or from HIDDs, be it pixelarrays
** or bitmaps. They use a callback-function to get data from amiga/put data to amiga
** bitmaps/pixelarrays
*/

/****************************************************************************************/

#if 0

void amiga2hidd_fast(APTR src_info, OOP_Object *hidd_gc, LONG x_src , LONG y_src,
    	    	     struct BitMap *hidd_bm, LONG x_dest, LONG y_dest,
		     ULONG xsize, ULONG ysize, VOID (*fillbuf_hook)(),
		     struct GfxBase * GfxBase)
{

    
    ULONG tocopy_w,
    	  tocopy_h;
	  
    LONG pixels_left_to_process = xsize * ysize;
	  
    LONG current_x, current_y, next_x, next_y;
    OOP_Object *bm_obj;

    next_x = 0;
    next_y = 0;
    
    bm_obj = OBTAIN_HIDD_BM(hidd_bm);
    if (NULL == bm_obj)
    	return;
    
    LOCK_PIXBUF 
      
    while (pixels_left_to_process)
    {

	/* Get some more pixels from the HIDD */

	current_x = next_x;
	current_y = next_y;
	
	if (NUMPIX < xsize)
	{
	   /* buffer can't hold a single horizontal line, and must 
	      divide each line into several copy-operations */
	    tocopy_w = xsize - current_x;
	    if (tocopy_w > NUMPIX)
	    {
	        /* Not quite finished with current horizontal pixel line */
	    	tocopy_w = NUMPIX;
		next_x += NUMPIX;
	    }
	    else
	    {	/* Start at a new line */
	    	next_x = 0;
		next_y ++;
	    }
	    tocopy_h = 1;
	    
    	}
	else /* We can copy one or several whole horizontal lines at a time */
	{
	    tocopy_h = MIN(NUMPIX / xsize, ysize - current_y);

	    tocopy_w = xsize;

	    next_x = 0;
	    next_y += tocopy_h;
	    
	
	}


	/* Get data */
	fillbuf_hook(src_info
		, current_x + x_src
		, current_y + y_src
		, current_x + x_dest
		, current_y + y_dest
		, tocopy_w, tocopy_h
		, PrivGBase(GfxBase)->pixel_buf
		, bm_obj
		, IS_HIDD_BM(hidd_bm) ? HIDD_BM_PIXTAB(hidd_bm) : NULL
		, GfxBase
	);

	/* Put it to the HIDD */
	D(bug("Putting box\n"));

	HIDD_BM_PutImage(bm_obj
		, hidd_gc
		, (UBYTE*)PrivGBase(GfxBase)->pixel_buf
		, tocopy_w * sizeof (HIDDT_Pixel)
		, x_dest + current_x
		, y_dest + current_y
		, tocopy_w, tocopy_h
		, vHidd_StdPixFmt_Native32
	);

	D(bug("Box put\n"));

	pixels_left_to_process -= (tocopy_w * tocopy_h);
	
	
    } /* while (pixels left to copy) */
    
    ULOCK_PIXBUF    

    RELEASE_HIDD_BM(bm_obj, hidd_bm);
    
    return;
    
}

#endif

/****************************************************************************************/

void hidd2buf_fast(struct BitMap *hidd_bm, LONG x_src , LONG y_src, APTR dest_info,
    	    	   LONG x_dest, LONG y_dest, ULONG xsize, ULONG ysize, VOID (*putbuf_hook)(),
		   struct GfxBase * GfxBase)
{

    ULONG tocopy_w, tocopy_h;
    
    LONG pixels_left_to_process = xsize * ysize;
    ULONG current_x, current_y, next_x, next_y;
    
    #warning Src bitmap migh be user initialized so we should not use HIDD_BM_PIXTAB() below
    
    OOP_Object *bm_obj;
    
    next_x = 0;
    next_y = 0;
    
    bm_obj = OBTAIN_HIDD_BM(hidd_bm);
    if (NULL == bm_obj)
    	return;
	
    LOCK_PIXBUF    

    while (pixels_left_to_process)
    {
	
	current_x = next_x;
	current_y = next_y;
	
	if (NUMPIX < xsize)
	{
	   /* buffer cant hold a single horizontal line, and must 
	      divide each line into copies */
	    tocopy_w = xsize - current_x;
	    if (tocopy_w > NUMPIX)
	    {
	        /* Not quite finished with current horizontal pixel line */
	    	tocopy_w = NUMPIX;
		next_x += NUMPIX;
	    }
	    else
	    {	/* Start at a new line */
	    
	    	next_x = 0;
		next_y ++;
	    }
	    tocopy_h = 1;
	    
    	}
    	else
    	{
	    tocopy_h = MIN(NUMPIX / xsize, ysize - current_y);
	    tocopy_w = xsize;

	    next_x = 0;
	    next_y += tocopy_h;
	    
    	}
	
	
	/* Get some more pixels from the HIDD */
	HIDD_BM_GetImage(bm_obj
		, (UBYTE *)PrivGBase(GfxBase)->pixel_buf
		, tocopy_w
		, x_src + current_x
		, y_src + current_y
		, tocopy_w, tocopy_h
		, vHidd_StdPixFmt_Native32);


	/*  Write pixels to the destination */
	putbuf_hook(dest_info
		, current_x + x_src
		, current_y + y_src
		, current_x + x_dest
		, current_y + y_dest
		, tocopy_w, tocopy_h
		, (HIDDT_Pixel *)PrivGBase(GfxBase)->pixel_buf
		, bm_obj
		, IS_HIDD_BM(hidd_bm) ? HIDD_BM_PIXTAB(hidd_bm) : NULL
	);
	
	pixels_left_to_process -= (tocopy_w * tocopy_h);

    }
    
    ULOCK_PIXBUF

    RELEASE_HIDD_BM(bm_obj, hidd_bm);
    
    return;
    
}

/****************************************************************************************/

UWORD hidd2cyber_pixfmt(HIDDT_StdPixFmt stdpf, struct GfxBase *GfxBase)
{
     UWORD cpf = (UWORD)-1;

     bug("hidd2cyber stdpf = %d [%d]\n", stdpf, vHidd_StdPixFmt_BGR032);

     switch (stdpf)
     {
	case vHidd_StdPixFmt_RGB15:
	    cpf = PIXFMT_RGB15;
	    break;

	case vHidd_StdPixFmt_RGB15_LE:
	    cpf = PIXFMT_RGB15PC;
	    break;

	case vHidd_StdPixFmt_BGR15:
	    cpf = PIXFMT_BGR15;
	    break;

	case vHidd_StdPixFmt_BGR15_LE:
	    cpf = PIXFMT_BGR15PC;
	    break;
	
	case vHidd_StdPixFmt_RGB16:
	    cpf = PIXFMT_RGB16;
	    break;

	case vHidd_StdPixFmt_RGB16_LE:
	    cpf = PIXFMT_RGB16PC;
	    break;

	case vHidd_StdPixFmt_BGR16:
	    cpf = PIXFMT_BGR16;
	    break;

	case vHidd_StdPixFmt_BGR16_LE:
	    cpf = PIXFMT_BGR16PC;
	    break;
	
	case vHidd_StdPixFmt_RGB24:
	    cpf = PIXFMT_RGB24;
	    break;

	case vHidd_StdPixFmt_BGR24:
	    cpf = PIXFMT_BGR24;
	    break;
	
	case vHidd_StdPixFmt_0RGB32:
	case vHidd_StdPixFmt_ARGB32:
	    cpf = PIXFMT_ARGB32;
	    break;
	
	case vHidd_StdPixFmt_RGB032:
	case vHidd_StdPixFmt_RGBA32:
	    cpf = PIXFMT_RGBA32;
	    break;
	
	case vHidd_StdPixFmt_BGRA32:
	case vHidd_StdPixFmt_BGR032:
	    cpf = PIXFMT_BGRA32;
	    break;
	    
	case vHidd_StdPixFmt_ABGR32:
	case vHidd_StdPixFmt_0BGR32:
	    cpf = PIXFMT_ABGR32;
	    break;
	
	case vHidd_StdPixFmt_LUT8:
	    cpf = PIXFMT_LUT8;
	    break;

	default:
	    D(bug("UNKNOWN CYBERGRAPHICS PIXFMT IN cyber2hidd_pixfmt\n"));
	    break;
     
    }

    return cpf;     
     
}

/****************************************************************************************/

HIDDT_StdPixFmt cyber2hidd_pixfmt(UWORD cpf, struct GfxBase *GfxBase)
{
    HIDDT_StdPixFmt stdpf = vHidd_StdPixFmt_Unknown;

    switch (cpf)
    {
	case PIXFMT_RGB15:
	    stdpf = vHidd_StdPixFmt_RGB15;
	    break;

	case PIXFMT_RGB15PC:
	    stdpf = vHidd_StdPixFmt_RGB15_LE;
	    break;

	case PIXFMT_BGR15:
	    stdpf = vHidd_StdPixFmt_BGR15;
	    break;

	case PIXFMT_BGR15PC:
	    stdpf = vHidd_StdPixFmt_BGR15_LE;
	    break;
	
	case PIXFMT_RGB16:
	    stdpf = vHidd_StdPixFmt_RGB16;
	    break;

	case PIXFMT_RGB16PC:
	    stdpf = vHidd_StdPixFmt_RGB16_LE;
	    break;

	case PIXFMT_BGR16:
	    stdpf = vHidd_StdPixFmt_BGR16;
	    break;

	case PIXFMT_BGR16PC:
	    stdpf = vHidd_StdPixFmt_BGR16_LE;
	    break;
	
	case PIXFMT_RGB24:
	    stdpf = vHidd_StdPixFmt_RGB24;
	    break;
	
	case PIXFMT_BGR24:
	    stdpf = vHidd_StdPixFmt_BGR24;
	    break;
	    
	case PIXFMT_ARGB32:
	    stdpf = vHidd_StdPixFmt_ARGB32;
	    break;
	
	case PIXFMT_RGBA32:
	    stdpf = vHidd_StdPixFmt_RGBA32;
	    break;
	
	case PIXFMT_BGRA32:
	    stdpf = vHidd_StdPixFmt_BGRA32;
	    break;
	
	case PIXFMT_ABGR32:
	    stdpf = vHidd_StdPixFmt_ABGR32;
	    break;
	
	case PIXFMT_LUT8:
	    stdpf = vHidd_StdPixFmt_LUT8;
	    break;

	default:
	    D(bug("UNKNOWN CYBERGRAPHICS PIXFMT IN cyber2hidd_pixfmt\n"));
	    break;
    }
    
    return stdpf;
}

/****************************************************************************************/

#define ENABLE_PROFILING   0
#define USE_OLD_MoveRaster 0

#define rdtscll(val) \
     __asm__ __volatile__("rdtsc" : "=A" (val))

#if ENABLE_PROFILING && defined(__i386__)


#define AROS_BEGIN_PROFILING(context)  \
{                                      \
    unsigned long long _time1, _time2; \
    char *_text = #context;            \
    rdtscll(_time1);                   \
    {

#define AROS_END_PROFILING                                                     \
    }                                                                          \
    rdtscll(_time2);                                                           \
    kprintf("%s: Ticks count: %u\n", _text, (unsigned long)(_time2 - _time1)); \
}

#else

#define AROS_BEGIN_PROFILING(context)
#define AROS_END_PROFILING

#endif

BOOL MoveRaster (struct RastPort * rp, LONG dx, LONG dy, LONG x1, LONG y1,
    	    	 LONG x2, LONG y2, BOOL UpdateDamageList, struct GfxBase * GfxBase)
{
    struct Layer     *L       = rp->Layer;
    struct Rectangle  ScrollRect;
    struct Rectangle  Rect;

    if (0 == dx && 0 == dy)
    	return TRUE;

    if (!OBTAIN_DRIVERDATA(rp, GfxBase))
	return FALSE;

    ScrollRect.MinX = x1;
    ScrollRect.MinY = y1;
    ScrollRect.MaxX = x2;
    ScrollRect.MaxY = y2;

    if (!L)
    {
        Rect = ScrollRect;
	TranslateRect(&Rect, -dx, -dy);
        if (_AndRectRect(&ScrollRect, &Rect, &Rect))
        {
            BltBitMap(rp->BitMap,
                      Rect.MinX + dx,
                      Rect.MinY + dy,
	              rp->BitMap,
                      Rect.MinX,
                      Rect.MinY,
                      Rect.MaxX - Rect.MinX + 1,
                      Rect.MaxY - Rect.MinY + 1,
		      0xc0, /* copy */
                      0xff,
                      NULL );
	}
    }
    else
    {
    	struct ClipRect *SrcCR;

	LockLayerRom(L);

        if (L->Flags & LAYERSIMPLE && UpdateDamageList)
        {
 	    /* Scroll the old damagelist within the scroll area */
	    ScrollRegion(L->DamageList, &ScrollRect, -dx, -dy);
	}

        /* The scrolling area is relative to the Layer, so make it relative to the screen */
        TranslateRect(&ScrollRect, MinX(L), MinY(L));

        /* The damage list will be formed by the now hidden layer's parts that will become visible due
           to the scrolling procedure, thus we procede this way:

           1) Calculate the invisible region out of the visible one, subtracting it from the
              scrolling area

           2) Scroll the invisible region by (-dx, -dy) and then subtract from it the not scrolled equivalent

           The regions that we obtain after (2) is the new damage list
        */

        if (L->Flags & LAYERSIMPLE && UpdateDamageList)
        {
	    Rect = ScrollRect;
            TranslateRect(&Rect, dx, dy);

	    if (_AndRectRect(&ScrollRect, &Rect, &Rect))
            {
 	        struct Region *Damage;

                Damage = NewRectRegion(Rect.MinX, Rect.MinY, Rect.MaxX, Rect.MaxY);
                if (Damage)
                {
        	    if
                    (
                        ClearRegionRegion(L->VisibleRegion, Damage)
                        &&
                        Damage->RegionRectangle
                    )
                    {
                        struct Region Tmp;
                        /*
                           We play sort of dirty here, by making assumptions about the internals of the
                           Region structure and the region handling functions, but we are allowed to do that,
                           aren't we? ;-)
      		        */

                        Tmp = *Damage;

                        TranslateRect(Bounds(Damage), -dx, -dy);

                        if
                        (
                            ClearRegionRegion(&Tmp, Damage)
                            &&
                            Damage->RegionRectangle
                        )
              	        {
		            /* Join the new damage list with the old one */
                	    TranslateRect(Bounds(Damage), -MinX(L), -MinY(L));
                	    OrRegionRegion(Damage, L->DamageList);

                	    L->Flags |= LAYERREFRESH;
            	        }
		    }

            	    DisposeRegion(Damage);
             	}
            }
        }

        AROS_BEGIN_PROFILING(SortLayerCR)

        #define LayersBase (struct LayersBase *)(GfxBase->gb_LayersBase)
	SortLayerCR(L, dx, dy);
	#undef LayersBase

        AROS_END_PROFILING

	AROS_BEGIN_PROFILING(Blitting loop)

#if USE_OLDMoveRaster

	{
            struct ClipREct *LastHiddenCR;

            for (LastHiddenCR = NULL, SrcCR = L->ClipRect; SrcCR; SrcCR = SrcCR->Next)
            {
                SrcCR->_p1 = LastHiddenCR;

                if (SrcCR->lobs)
                    LastHiddenCR = SrcCR;
	    }
        }


        for (SrcCR = L->ClipRect; SrcCR; SrcCR = SrcCR->Next)
    	{
 	    int cando = 0;

            if (SrcCR->lobs && (L->Flags & LAYERSIMPLE))
	    {
                continue;
	    }

            if (_AndRectRect(&ScrollRect, Bounds(SrcCR), &Rect))
	    {
		TranslateRect(&Rect, -dx, -dy);

		if (_AndRectRect(&ScrollRect, &Rect, &Rect))
		    cando = 1;
	    }

	    if (cando)
	    {
		/* Rect.Min(X|Y) are the coordinates to wich the rectangle has to be moved
		   Rect.Max(X|Y) - Rect.Max(X|Y) - 1 are the dimensions of this rectangle */
		if (!SrcCR->_p1 && !SrcCR->lobs)
		{
		    /* there are no hidden/obscured rectangles this recrtangle has to deal with*/
		    BltBitMap
                    (
                        rp->BitMap,
                        Rect.MinX + dx,
        		Rect.MinY + dy,
	          	rp->BitMap,
                    	Rect.MinX,
                   	Rect.MinY,
                  	Rect.MaxX - Rect.MinX + 1,
                  	Rect.MaxY - Rect.MinY + 1,
			0xc0, /* copy */
         		0xff,
                 	NULL
 		    );
		}
		else
		{
		    struct BitMap          *srcbm;
		    struct RegionRectangle *rr;
                    struct Region          *RectRegion;
		    struct Rectangle        Tmp;
		    struct ClipRect        *HiddCR;
		    WORD                    corrsrcx, corrsrcy;
		    BOOL   dosrcsrc;

		    RectRegion = NewRectRegion(Rect.MinX, Rect.MinY, Rect.MaxX, Rect.MaxY);
		    if (!RectRegion)
		        goto failexit;

 		    if (SrcCR->lobs)
		    {
			if (L->Flags & LAYERSUPER)
		        {
   		            corrsrcx = - MinX(L) - L->Scroll_X;
          	            corrsrcy = - MinY(L) - L->Scroll_Y;
		        }
			else
			{
		            corrsrcx = - MinX(SrcCR) + ALIGN_OFFSET(MinX(SrcCR));
		            corrsrcy = - MinY(SrcCR);
		        }
			srcbm = SrcCR->BitMap;
		    }
		    else
		    {
		        corrsrcx  = 0;
		        corrsrcy  = 0;
		        srcbm     = rp->BitMap;
		    }

		    for (HiddCR = SrcCR->_p1; HiddCR; HiddCR = HiddCR->_p1)
		    {
			if (_AndRectRect(Bounds(RectRegion), Bounds(HiddCR), &Tmp))
			{
			    if (!(L->Flags & LAYERSIMPLE))
			    {
    			        WORD corrdstx, corrdsty;

				if (L->Flags & LAYERSUPER)
				{
	                            corrdstx =  - MinX(L) - L->Scroll_X;
                        	    corrdsty =  - MinY(L) - L->Scroll_Y;
				}
				else
				{
				    /* Smart layer */
				    corrdstx =  - MinX(HiddCR) + ALIGN_OFFSET(MinX(HiddCR));
				    corrdsty =  - MinY(HiddCR);
				}


				BltBitMap
                                (
                                    srcbm,
				    Tmp.MinX + corrsrcx + dx,
				    Tmp.MinY + corrsrcy + dy,
				    HiddCR->BitMap,
				    Tmp.MinX + corrdstx,
				    Tmp.MinY + corrdsty,
				    Tmp.MaxX - Tmp.MinX + 1,
                	      	    Tmp.MaxY - Tmp.MinY + 1,
			      	    0xc0, /* copy */
         		      	    0xff,
                 	   	    NULL
                                );
			    }

			    if (!ClearRectRegion(RectRegion, &Tmp))
			    {
			        DisposeRegion(RectRegion);
				goto failexit;
			    }
			}
		    }

		    if ((dosrcsrc = _AndRectRect(Bounds(SrcCR), &Rect, &Tmp)))
		    {
			if (!ClearRectRegion(RectRegion, &Tmp))
			{
			    DisposeRegion(RectRegion);
			    goto failexit;
			}
		    }

		    for (rr = RectRegion->RegionRectangle; rr; rr = rr->Next)
		    {
			BltBitMap
                        (
                            srcbm,
			    MinX(rr) + MinX(RectRegion) + corrsrcx + dx,
                	    MinY(rr) + MinY(RectRegion) + corrsrcy + dy,
	          	    rp->BitMap,
                	    MinX(rr) + MinX(RectRegion),
        		    MinY(rr) + MinY(RectRegion),
                	    Width(rr),
                	    Height(rr),
			    0xc0, /* copy */
         		    0xff,
                 	    NULL
                        );
		    }

                    if (dosrcsrc)
		    {
			BltBitMap
                        (
                            srcbm,
			    Tmp.MinX + corrsrcx + dx,
                	    Tmp.MinY + corrsrcy + dy,
	      		    srcbm,
			    Tmp.MinX + corrsrcx,
                	    Tmp.MinY + corrsrcy,
       			    Tmp.MaxX - Tmp.MinX + 1,
                	    Tmp.MaxY - Tmp.MinY + 1,
			    0xc0, /* copy */
         		    0xff,
                 	    NULL
                        );

		    }

                    DisposeRegion(RectRegion);
		}
	    }
        }

#else

        for (SrcCR = L->ClipRect; SrcCR; SrcCR = SrcCR->Next)
    	{
            if (_AndRectRect(&ScrollRect, Bounds(SrcCR), &Rect))
	    {
		TranslateRect(&Rect, -dx, -dy);

		if (_AndRectRect(&ScrollRect, &Rect, &Rect))
                {
                    struct BitMap   *srcbm;
		    struct ClipRect *DstCR;
                    LONG             corrsrcx, corrsrcy;
                    ULONG            area;

                    if (SrcCR->lobs)
	            {
		        if (L->Flags & LAYERSIMPLE) continue;

                	if (L->Flags & LAYERSUPER)
	        	{
	            	    corrsrcx = - MinX(L) - L->Scroll_X;
       	            	    corrsrcy = - MinY(L) - L->Scroll_Y;
	        	}
			else
			{
	            	    corrsrcx = - MinX(SrcCR) + ALIGN_OFFSET(MinX(SrcCR));
	            	    corrsrcy = - MinY(SrcCR);
	  		}
			srcbm = SrcCR->BitMap;
	  	    }
	    	    else
	    	    {
	       		corrsrcx  = 0;
	        	corrsrcy  = 0;
	        	srcbm = rp->BitMap;
	    	    }

                    area = (ULONG)(Rect.MaxX - Rect.MinX + 1) * (ULONG)(Rect.MaxY - Rect.MinY + 1);

            	    for (DstCR = L->ClipRect ; area && DstCR; DstCR = DstCR->Next)
            	    {
		  	struct Rectangle Rect2;

                        if (_AndRectRect(Bounds(DstCR), &Rect, &Rect2))
 			{
                            struct BitMap   *dstbm;
	                    LONG             corrdstx, corrdsty;

                            area -= (ULONG)(Rect2.MaxX - Rect2.MinX + 1) * (ULONG)(Rect2.MaxY - Rect2.MinY + 1);

                            if (DstCR->lobs)
	                    {
		                if (L->Flags & LAYERSIMPLE) continue;

                    	        if (L->Flags & LAYERSUPER)
 	          	        {
	            	            corrdstx = - MinX(L) - L->Scroll_X;
       	            	            corrdsty = - MinY(L) - L->Scroll_Y;
	          	        }
			        else
		  	        {
	            	            corrdstx = - MinX(DstCR) + ALIGN_OFFSET(MinX(DstCR));
	            	            corrdsty = - MinY(DstCR);
	  		        }
			        dstbm = DstCR->BitMap;
	  	            }
	    	            else
	    	            {
	       		        corrdstx  = 0;
	        	        corrdsty  = 0;
	        	        dstbm = rp->BitMap;
	    	            }

                            BltBitMap
                            (
                                srcbm,
                                Rect2.MinX + corrsrcx + dx,
                                Rect2.MinY + corrsrcy + dy,
                                dstbm,
                                Rect2.MinX + corrdstx,
                                Rect2.MinY + corrdsty,
				Rect2.MaxX - Rect2.MinX + 1,
				Rect2.MaxY - Rect2.MinY + 1,
                                0xC0,
                                0xFF,
                                NULL
			    );
			}
            	    }
        	}
	    }
 	}
#endif
        AROS_END_PROFILING

        UnlockLayerRom(L);
    }

    RELEASE_DRIVERDATA(rp, GfxBase);
    
    return TRUE;
}

/****************************************************************************************/
