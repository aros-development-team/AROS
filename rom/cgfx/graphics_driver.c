/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id: graphics_driver.c 32374 2010-01-18 07:45:12Z sonic $

    Desc: Driver for using gfxhidd for gfx output
    Lang: english
*/


#include <proto/cybergraphics.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/arossupport.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <exec/memory.h>
#include <exec/semaphores.h>
#include <clib/macros.h>

#include <graphics/rastport.h>
#include <graphics/gfxbase.h>
#include <graphics/text.h>
#include <graphics/view.h>
#include <graphics/layers.h>
#include <graphics/clip.h>
#include <graphics/gfxmacros.h>
#include <graphics/regions.h>
#include <graphics/scale.h>

#include <oop/oop.h>
#include <utility/tagitem.h>
#include <aros/asmcall.h>

#include <intuition/intuition.h>

#include <hidd/graphics.h>

#include <cybergraphx/cybergraphics.h>

#include <stdio.h>
#include <string.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"
#include "cybergraphics_intern.h"

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

struct rgbpix_render_data
{
    HIDDT_Pixel pixel;
};


static LONG rgbpix_write(APTR pr_data, OOP_Object *bm, OOP_Object *gc,
    	            	 LONG x, LONG y, struct GfxBase *GfxBase)
{
    struct rgbpix_render_data *prd;
    
    prd = (struct rgbpix_render_data *)pr_data;
    
    HIDD_BM_PutPixel(bm, x, y, prd->pixel);
    
    return 0;
}

struct wpa_render_data
{
    UBYTE *array;
    HIDDT_StdPixFmt pixfmt;
    ULONG modulo;
    ULONG bppix;
};

static ULONG wpa_render(APTR wpar_data
	, LONG srcx, LONG srcy
	, OOP_Object *dstbm_obj
	, OOP_Object *dst_gc
	, LONG x1, LONG y1, LONG x2, LONG y2
	, struct GfxBase *GfxBase)
{
    struct wpa_render_data *wpard;
    ULONG width, height;
    UBYTE *array;
    
    width  = x2 - x1 + 1;
    height = y2 - y1 + 1;
    
    wpard = (struct wpa_render_data *)wpar_data;
    
    array = wpard->array + wpard->modulo * srcy + wpard->bppix * srcx;
    
    HIDD_BM_PutImage(dstbm_obj
    	, dst_gc, array
	, wpard->modulo
	, x1, y1
	, width, height
	, wpard->pixfmt
    );
    
    return width * height;
}

struct wpaa_render_data
{
    UBYTE *array;
    ULONG modulo;
};

static ULONG wpaa_render(APTR wpaar_data
	, LONG srcx, LONG srcy
	, OOP_Object *dstbm_obj
	, OOP_Object *dst_gc
	, LONG x1, LONG y1, LONG x2, LONG y2
	, struct GfxBase *GfxBase)
{
    struct wpaa_render_data *wpaard;
    ULONG   	    	     width, height;
    UBYTE   	    	    *array;
    
    width  = x2 - x1 + 1;
    height = y2 - y1 + 1;
    
    wpaard = (struct wpaa_render_data *)wpaar_data;
    
    array = wpaard->array + wpaard->modulo * srcy + 4 * srcx;
    
    HIDD_BM_PutAlphaImage(dstbm_obj
    	, dst_gc, array
	, wpaard->modulo
	, x1, y1
	, width, height
    );
    
    return width * height;
}

struct bta_render_data
{
    UBYTE *array;
    ULONG  modulo;
    UBYTE  invertalpha;
};

static ULONG bta_render(APTR bta_data
	, LONG srcx, LONG srcy
	, OOP_Object *dstbm_obj
	, OOP_Object *dst_gc
	, LONG x1, LONG y1, LONG x2, LONG y2
	, struct GfxBase *GfxBase)
{
    struct bta_render_data *btard;
    ULONG   	    	    width, height;
    UBYTE   	    	   *array;
    
    width  = x2 - x1 + 1;
    height = y2 - y1 + 1;
    
    btard = (struct bta_render_data *)bta_data;
    
    array = btard->array + btard->modulo * srcy + srcx;
    
    HIDD_BM_PutAlphaTemplate(dstbm_obj
    	, dst_gc, array
	, btard->modulo
	, x1, y1
	, width, height
	, btard->invertalpha
    );
    
    return width * height;
}

struct rpa_render_data {
    UBYTE *array;
    HIDDT_StdPixFmt pixfmt;
    ULONG modulo;
    ULONG bppix;
};

static ULONG rpa_render(APTR rpar_data
	, LONG srcx, LONG srcy
	, OOP_Object *dstbm_obj
	, OOP_Object *dst_gc
	, LONG x1, LONG y1, LONG x2, LONG y2
	, struct GfxBase *GfxBase)
{
    struct rpa_render_data *rpard;
    ULONG width, height;
    UBYTE *array;
    
    width  = x2 - x1 + 1;
    height = y2 - y1 + 1;
    
    rpard = (struct rpa_render_data *)rpar_data;
    
    array = rpard->array + rpard->modulo * srcy + rpard->bppix * srcx;
    
    HIDD_BM_GetImage(dstbm_obj
    	, array
	, rpard->modulo
	, x1, y1
	, width, height
	, rpard->pixfmt
    );
    
    return width * height;
}

static LONG pix_read(APTR pr_data
	, OOP_Object *bm, OOP_Object *gc
	, LONG x, LONG y
	, struct GfxBase *GfxBase)
{
    struct rgbpix_render_data *prd;
    
    prd = (struct rgbpix_render_data *)pr_data;
    
    prd->pixel = HIDD_BM_GetPixel(bm, x, y);

    
    return 0;
}


struct extcol_render_data
{
    struct BitMap *curbm;
    struct BitMap *destbm;
    HIDDT_Pixel pixel;
    struct IntCGFXBase *CyberGfxBase;
};


static VOID buf_to_extcol(struct extcol_render_data *ecrd
	, LONG srcx, LONG srcy
	, LONG dstx, LONG dsty
	, ULONG width, ULONG height
	, HIDDT_Pixel *pixbuf
	, OOP_Object *bm_obj
	, HIDDT_Pixel *pixtab)
{
    LONG y;
    struct BitMap *bm;
    bm = ecrd->destbm;
    for (y = 0; y < height; y ++) {
    	LONG x;
	
    	for (x = 0; x < width; x ++) {
	    if (*pixbuf ++ == ecrd->pixel) {
	    	
	    	UBYTE *plane;
		ULONG i;
	    	/* Set the according bit in the bitmap */
		for (i = 0; i < bm->Depth; i ++) {
		    plane = bm->Planes[i];
		    if (NULL != plane) {
		    	UBYTE mask;
			
			plane += COORD_TO_BYTEIDX(x + dstx, y + dsty, bm->BytesPerRow);
			mask = XCOORD_TO_MASK(x + dstx);
			
			/* Set the pixel */
			*plane |= mask;
		    
		    } /* if (plane allocated) */
		} /* for (plane) */
	    } /* if (color match) */
	} /* for (x) */
    } /* for (y) */
    
    return;
}

static ULONG extcol_render(APTR funcdata
	, LONG dstx, LONG dsty
	, OOP_Object *dstbm_obj
	, OOP_Object *dst_gc
	, LONG x1, LONG y1, LONG x2, LONG y2
	, struct GfxBase *GfxBase)
{
    /* Get the info from the hidd */
    struct extcol_render_data *ecrd = (struct extcol_render_data *)funcdata;
    struct IntCGFXBase *CyberGfxBase = ecrd->CyberGfxBase;
     
    hidd2buf_fast(ecrd->curbm
     	, x1, y1
	, (APTR)ecrd
	, dstx, dsty
	, x2 - x1 + 1
	, y2 - y1 + 1
	, buf_to_extcol
	, CyberGfxBase
    );
		
    return (x2 - x1 + 1) * (y2 - y1 + 1);
}

struct dm_message {
    APTR memptr;
    ULONG offsetx;
    ULONG offsety;
    ULONG xsize;
    ULONG ysize;
    UWORD bytesperrow;
    UWORD bytesperpix;
    UWORD colormodel;
    
};

struct dm_render_data {
    struct dm_message msg;
    OOP_Object *pf;
    struct Hook *hook;
    struct RastPort *rp;
    IPTR stdpf;
    struct IntCGFXBase *CyberGfxBase;
};


static ULONG dm_render(APTR dmr_data
	, LONG srcx, LONG srcy
	, OOP_Object *dstbm_obj
	, OOP_Object *dst_gc
	, LONG x1, LONG y1, LONG x2, LONG y2
	, struct GfxBase *GfxBase)
{
    struct dm_render_data *dmrd = (struct dm_render_data *)dmr_data;
    struct IntCGFXBase *CyberGfxBase = dmrd->CyberGfxBase;
    UBYTE *addr;
    struct dm_message *msg;
    IPTR bytesperpixel;
    ULONG width, height, fb_width, fb_height;
    ULONG banksize, memsize;
    
    width  = x2 - x1 + 1;
    height = y2 - y1 + 1;;
    msg = &dmrd->msg;
#if 1
    msg->offsetx = x1;
    msg->offsety = y1;
#else
    #warning "Not sure about this one . Set it to 0 since we adjust msg->memptr to x1/y1 lower down"
    msg->offsetx = 0; // x1;
    msg->offsety = 0; // y1;
#endif
    msg->xsize = width;
    msg->ysize = height;
    
    /* Get the baseadress from where to render */
    if (HIDD_BM_ObtainDirectAccess(dstbm_obj
    	, &addr
	, &fb_height, &fb_width
	, &banksize, &memsize)) {

	OOP_GetAttr(dmrd->pf, aHidd_PixFmt_BytesPerPixel, &bytesperpixel);
	msg->bytesperpix = (UWORD)bytesperpixel;
    
	/* Colormodel allready set */
    
	/* Compute the adress for the start pixel */
	#warning "We should maybe use something else than the BytesPerLine method since we may have alignment"
	msg->bytesperrow = HIDD_BM_BytesPerLine(dstbm_obj, dmrd->stdpf, width);
#if 1
	msg->memptr = addr;
#else
	msg->memptr = addr + (msg->bytesperrow * y1) + (bytesperpixel * x1);
#endif
	
	HIDD_BM_ReleaseDirectAccess(dstbm_obj);
	
	CallHookPkt(dmrd->hook, dmrd->rp, msg);
	
    } else {
    	/* We are unable to gain direct access to the framebuffer,
	   so we have to emulate it
	*/
	struct TagItem gc_tags[] = {
	    { aHidd_GC_DrawMode, vHidd_GC_DrawMode_Copy },
	    { TAG_DONE, 0UL }
	};
	ULONG bytesperrow;
	ULONG tocopy_h, max_tocopy_h;
	ULONG lines_todo;
	OOP_Object *gfxhidd, *gc;
    
	lines_todo = height;
    
	/* The HIDD bm does not have a base adress so we have to render into
	   it using a temporary buffer
	*/
   	OOP_GetAttr(dmrd->pf, aHidd_PixFmt_BytesPerPixel, &bytesperpixel);
	//bytesperrow = HIDD_BM_BytesPerLine(dstbm_obj, dmrd->stdpf, width);
	bytesperrow = width * bytesperpixel;


	D(kprintf("width %d bytesperrow %d bytesperpixel %d\n", width, bytesperrow, bytesperpixel));
	D(kprintf(" colormodel %d\n", msg->colormodel));

	if (PIXELBUF_SIZE < bytesperrow) {
	    D(bug("!!! NOT ENOUGH SPACE IN TEMP BUFFER FOR A SINGLE LINE IN DoCDrawMethodTagList() !!!\n"));
	    return 0;
    	}
    
    	/* Calculate number of lines we might copy */
    	max_tocopy_h = PIXELBUF_SIZE / bytesperrow;
    
    #if 1
    	msg->offsetx = 0;
	msg->offsety = 0;
    #endif
    
	OOP_GetAttr(dstbm_obj, aHidd_BitMap_GfxHidd, (IPTR *)&gfxhidd);
	gc = HIDD_Gfx_NewGC(gfxhidd, gc_tags);
    
    	/* Get the maximum number of lines */
    	while (lines_todo != 0) {
    	    tocopy_h = MIN(lines_todo, max_tocopy_h);
    	    msg->memptr = CyberGfxBase->pixel_buf;
	    msg->bytesperrow = bytesperrow;
    
	    msg->bytesperpix = (UWORD)bytesperpixel;

LOCK_PIXBUF
	    HIDD_BM_GetImage(dstbm_obj
		, (UBYTE *)CyberGfxBase->pixel_buf
		, bytesperrow
		, x1, y1 + height - lines_todo, width, lines_todo
		, dmrd->stdpf
	    );
    	    
	    /* Use the hook to set some pixels */
	    CallHookPkt(dmrd->hook, dmrd->rp, msg);

	    HIDD_BM_PutImage(dstbm_obj, gc
		, (UBYTE *)CyberGfxBase->pixel_buf
		, bytesperrow
		, x1, y1 + height - lines_todo, width, lines_todo
		, dmrd->stdpf
	    );

ULOCK_PIXBUF

    	    lines_todo -= tocopy_h;
	}

	OOP_DisposeObject(gc);
    }
    
    return width * height;
}



LONG driver_WriteLUTPixelArray(APTR srcrect,
	UWORD srcx, UWORD srcy,
	UWORD srcmod, struct RastPort *rp, APTR ctable,
	UWORD destx, UWORD desty,
	UWORD sizex, UWORD sizey,
	UBYTE ctabformat,
	struct IntCGFXBase *CyberGfxBase)
{
    ULONG depth;
    
    HIDDT_PixelLUT pixlut;
    HIDDT_Pixel pixtab[256];
    
    HIDDT_Color col;
    ULONG i;
    
    LONG pixwritten = 0;
    UBYTE *buf;
    
    /* This is cybergraphx. We only work wih HIDD bitmaps */
    if (!IS_HIDD_BM(rp->BitMap)) {
    	D(bug("!!!!! Trying to use CGFX call on non-hidd bitmap in WriteLUTPixelArray()!!!\n"));
    	return 0;
    }
    
    pixlut.entries	= 256;
    pixlut.pixels	= pixtab;
    
    depth = GetBitMapAttr(rp->BitMap, BMA_DEPTH);
    
    /* This call does only support bitmaps with depth > 8. Use WritePixelArray8
       for other bitmaps
    */
    
    if (depth <= 8) {
    	D(bug("!!! TRYING TO USE WriteLUTPixelArray() ON BITMAP WITH DEPTH < 8\n"));
    	return 0;
    }
	
    /* Curently only one format is supported */
    if (CTABFMT_XRGB8 != ctabformat) {
    	D(bug("!!! WriteLUTPixelArray() CALLED WITH UNSUPPORTED CTAB FORMAT %d\n"
		, ctabformat));
    	return 0;
    }
    col.alpha	= 0;
	
    /* Convert the coltab into native pixels */
    for (i = 0; i < 256; i ++) {
    	register ULONG rgb = ((ULONG *)ctable)[i];
    	col.red		= (HIDDT_ColComp)((rgb & 0x00FF0000) >> 8);
	col.green	= (HIDDT_ColComp)(rgb & 0x0000FF00);
	col.blue	= (HIDDT_ColComp)((rgb & 0x000000FF) << 8);
	
	pixtab[i] = HIDD_BM_MapColor(HIDD_BM_OBJ(rp->BitMap), &col);
    }
    
    buf = (UBYTE *)srcrect;
    
    buf += CHUNKY8_COORD_TO_BYTEIDX(srcx, srcy, srcmod);
    
    pixwritten = WritePixels8(rp
    	, buf
	, srcmod
	, destx, desty
	, destx + sizex - 1, desty + sizey - 1
	, &pixlut
        , TRUE
    );
    
    /* Now blit the colors onto the screen */
    
    return pixwritten;
}


LONG driver_WritePixelArray(APTR src, UWORD srcx, UWORD srcy
	, UWORD srcmod, struct RastPort *rp, UWORD destx, UWORD desty
	, UWORD width, UWORD height, UBYTE srcformat, struct IntCGFXBase *CyberGfxBase)
{
     
    OOP_Object *pf = 0;
    HIDDT_StdPixFmt srcfmt_hidd = 0, morphfmt_hidd = 0;
    ULONG start_offset;
    IPTR bppix;
    
    LONG pixwritten = 0;
    
    struct wpa_render_data wpard;
    struct Rectangle rr;

    if (RECTFMT_GREY8 == srcformat)
    {
    	static ULONG greytab[256];
	
	/* Ignore possible race conditions during
	   initialization. Have no bad effect. Just
	   double initializations. */
	   
	/* FIXME/KILLME: evil static array which goes into BSS section
	   which x86 native AROS regards as evil! */
	   
	if (greytab[255] == 0)
	{
	    WORD i;
	    
	    for(i = 0; i < 256; i++)
	    {
	    	greytab[i] = i * 0x010101;
	    }
	}
	
	return WriteLUTPixelArray(src, srcx, srcy, srcmod,
	    	    	    	    	 rp, greytab, destx, desty,
					 width, height, CTABFMT_XRGB8);
    }

    /* This is cybergraphx. We only work wih HIDD bitmaps */
    if (!IS_HIDD_BM(rp->BitMap))
    {
    	D(bug("!!!!! Trying to use CGFX call on non-hidd bitmap in WritePixelArray() !!!\n"));
    	return 0;
    }
    
    if (RECTFMT_LUT8 == srcformat)
    {
	HIDDT_PixelLUT pixlut = { 256, HIDD_BM_PIXTAB(rp->BitMap) };
	UBYTE * array = (UBYTE *)src;

	if (!HIDD_BM_PIXTAB(rp->BitMap))
	{
	    D(bug("!!! No CLUT in driver_WritePixelArray\n"));
	    return 0;
	}

	array += CHUNKY8_COORD_TO_BYTEIDX(srcx, srcy, srcmod);
	
    	pixwritten = WritePixels8(rp
		, array, srcmod
		, destx, desty
		, destx + width - 1, desty + height - 1
		, &pixlut
                , TRUE);

	return pixwritten;
    }
        
    switch (srcformat)
    {
    	case RECTFMT_RGB15  : srcfmt_hidd = vHidd_StdPixFmt_RGB15   ; break;
    	case RECTFMT_BGR15  : srcfmt_hidd = vHidd_StdPixFmt_BGR15   ; break;
    	case RECTFMT_RGB15PC: srcfmt_hidd = vHidd_StdPixFmt_RGB15_LE; break;
    	case RECTFMT_BGR15PC: srcfmt_hidd = vHidd_StdPixFmt_BGR15_LE; break;
    	case RECTFMT_RGB16  : srcfmt_hidd = vHidd_StdPixFmt_RGB16   ; break;
    	case RECTFMT_BGR16  : srcfmt_hidd = vHidd_StdPixFmt_BGR16   ; break;
    	case RECTFMT_RGB16PC: srcfmt_hidd = vHidd_StdPixFmt_RGB16_LE; break;
    	case RECTFMT_BGR16PC: srcfmt_hidd = vHidd_StdPixFmt_BGR16_LE; break;
	case RECTFMT_RGB24  : srcfmt_hidd = vHidd_StdPixFmt_RGB24   ; break;
    	case RECTFMT_BGR24  : srcfmt_hidd = vHidd_StdPixFmt_BGR24   ; break;
    	case RECTFMT_0RGB32 : srcfmt_hidd = vHidd_StdPixFmt_0RGB32  ; break;
    	case RECTFMT_BGR032 : srcfmt_hidd = vHidd_StdPixFmt_BGR032  ; break;
    	case RECTFMT_RGB032 : srcfmt_hidd = vHidd_StdPixFmt_RGB032  ; break;
    	case RECTFMT_0BGR32 : srcfmt_hidd = vHidd_StdPixFmt_0BGR32  ; break;
	case RECTFMT_ARGB32 : srcfmt_hidd = vHidd_StdPixFmt_ARGB32  ; morphfmt_hidd = vHidd_StdPixFmt_0RGB32; break;
    	case RECTFMT_BGRA32 : srcfmt_hidd = vHidd_StdPixFmt_BGRA32  ; morphfmt_hidd = vHidd_StdPixFmt_BGR032; break;
	case RECTFMT_RGBA32 : srcfmt_hidd = vHidd_StdPixFmt_RGBA32  ; morphfmt_hidd = vHidd_StdPixFmt_RGB032; break;
	case RECTFMT_ABGR32 : srcfmt_hidd = vHidd_StdPixFmt_ABGR32  ; morphfmt_hidd = vHidd_StdPixFmt_0BGR32; break;
	case RECTFMT_RAW  : srcfmt_hidd = vHidd_StdPixFmt_Native; break;
    }

    /* Compute the start of the array */

#warning Get rid of the below code ?
/* This can be done by passing the srcx and srcy parameters on to
   the HIDD bitmap and let it take care of it itself.
   This means that HIDD_BM_PutImage() gets a lot of parameters,
   which may not be necessary in real life.
   
   Compromise: convert from *CyberGfx* pixfmt to bppix using a table lookup.
   This is faster
*/
    if ((srcfmt_hidd == vHidd_StdPixFmt_Native) || (morphfmt_hidd != 0))
    {
    	OOP_GetAttr(HIDD_BM_OBJ(rp->BitMap), aHidd_BitMap_PixFmt, (IPTR *)&pf);
    }
    
    if (srcfmt_hidd != vHidd_StdPixFmt_Native)
    {
    	/* RECTFMT_ARGB32 on vHidd_StdPixFmt_0RGB32 bitmap ==> use vHidd_StdPixFmt_0RGB32 */
    	/* RECTFMT_BGRA32 on vHidd_StdPixFmt_BGR032 bitmap ==> use vHidd_StdPixFmt_BGR032 */
    	/* RECTFMT_RGBA32 on vHidd_StdPixFmt_RGB032 bitmap ==> use vHidd_StdPixFmt_RGB032 */
    	/* RECTFMT_ABGR32 on vHidd_StdPixFmt_0BGR32 bitmap ==> use vHidd_StdPixFmt_0BGR32 */
	OOP_Object *gfxhidd;
	
    	if (morphfmt_hidd != 0)
	{
	    IPTR stdpf;

	    OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, (IPTR *)&stdpf);	    
	    if (stdpf == morphfmt_hidd) srcfmt_hidd = morphfmt_hidd;
        }

	OOP_GetAttr(HIDD_BM_OBJ(rp->BitMap), aHidd_BitMap_GfxHidd, (IPTR *)&gfxhidd);
    	pf = HIDD_Gfx_GetPixFmt(gfxhidd, srcfmt_hidd);
    }
        
    OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, &bppix);
    
    start_offset = ((ULONG)srcy) * srcmod + srcx * bppix;
        
    wpard.array	 = ((UBYTE *)src) + start_offset;
    wpard.pixfmt = srcfmt_hidd;
    wpard.modulo = srcmod;
    wpard.bppix	 = bppix;
    
    rr.MinX = destx;
    rr.MinY = desty;
    rr.MaxX = destx + width  - 1;
    rr.MaxY = desty + height - 1;
    
    pixwritten = DoRenderFunc(rp, NULL, &rr, wpa_render, &wpard, TRUE);

    return pixwritten;
}

LONG driver_WritePixelArrayAlpha(APTR src, UWORD srcx, UWORD srcy
	, UWORD srcmod, struct RastPort *rp, UWORD destx, UWORD desty
	, UWORD width, UWORD height, ULONG globalalpha, struct IntCGFXBase *CyberGfxBase)
{
    ULONG   	    	    start_offset;    
    LONG    	    	    pixwritten = 0;    
    struct wpaa_render_data wpaard;
    struct Rectangle 	    rr;

    /* This is cybergraphx. We only work wih HIDD bitmaps */
    if (!IS_HIDD_BM(rp->BitMap))
    {
    	D(bug("!!!!! Trying to use CGFX call on non-hidd bitmap in WritePixelArrayAlpha() !!!\n"));
    	return 0;
    }
    
    /* Compute the start of the array */

    start_offset = ((ULONG)srcy) * srcmod + srcx * 4;
        
    wpaard.array  = ((UBYTE *)src) + start_offset;
    wpaard.modulo = srcmod;
    
    rr.MinX = destx;
    rr.MinY = desty;
    rr.MaxX = destx + width  - 1;
    rr.MaxY = desty + height - 1;
    
    pixwritten = DoRenderFunc(rp, NULL, &rr, wpaa_render, &wpaard, TRUE);
    
    return pixwritten;
}

LONG driver_ReadPixelArray(APTR dst, UWORD destx, UWORD desty
	, UWORD dstmod, struct RastPort *rp, UWORD srcx, UWORD srcy
	, UWORD width, UWORD height, UBYTE dstformat, struct IntCGFXBase *CyberGfxBase)
{
     
    OOP_Object *pf = 0;    
    HIDDT_StdPixFmt dstfmt_hidd = 0, morphfmt_hidd = 0;
    
    ULONG start_offset;
    IPTR bppix;
    
    LONG pixread = 0;
    BYTE old_drmd;
    
    struct Rectangle rr;
    struct rpa_render_data rpard;

    /* This is cybergraphx. We only work wih HIDD bitmaps */
    if (!IS_HIDD_BM(rp->BitMap))
    {
    	D(bug("!!!!! Trying to use CGFX call on non-hidd bitmap in ReadPixelArray() !!!\n"));
    	return 0;
    }
    
    /* Preserve old drawmode */
    old_drmd = rp->DrawMode;
    SetDrMd(rp, JAM2);
    
    switch (dstformat)
    {
    	case RECTFMT_RGB15  : dstfmt_hidd = vHidd_StdPixFmt_RGB15   ; break;
    	case RECTFMT_BGR15  : dstfmt_hidd = vHidd_StdPixFmt_BGR15   ; break;
    	case RECTFMT_RGB15PC: dstfmt_hidd = vHidd_StdPixFmt_RGB15_LE; break;
    	case RECTFMT_BGR15PC: dstfmt_hidd = vHidd_StdPixFmt_BGR15_LE; break;
    	case RECTFMT_RGB16  : dstfmt_hidd = vHidd_StdPixFmt_RGB16   ; break;
    	case RECTFMT_BGR16  : dstfmt_hidd = vHidd_StdPixFmt_BGR16   ; break;
    	case RECTFMT_RGB16PC: dstfmt_hidd = vHidd_StdPixFmt_RGB16_LE; break;
    	case RECTFMT_BGR16PC: dstfmt_hidd = vHidd_StdPixFmt_BGR16_LE; break;
	case RECTFMT_RGB24  : dstfmt_hidd = vHidd_StdPixFmt_RGB24   ; break;
    	case RECTFMT_BGR24  : dstfmt_hidd = vHidd_StdPixFmt_BGR24   ; break;
    	case RECTFMT_0RGB32 : dstfmt_hidd = vHidd_StdPixFmt_0RGB32  ; break;
    	case RECTFMT_BGR032 : dstfmt_hidd = vHidd_StdPixFmt_BGR032  ; break;
    	case RECTFMT_RGB032 : dstfmt_hidd = vHidd_StdPixFmt_RGB032  ; break;
    	case RECTFMT_0BGR32 : dstfmt_hidd = vHidd_StdPixFmt_0BGR32  ; break;
	case RECTFMT_ARGB32 : dstfmt_hidd = vHidd_StdPixFmt_ARGB32  ; morphfmt_hidd = vHidd_StdPixFmt_0RGB32; break;
    	case RECTFMT_BGRA32 : dstfmt_hidd = vHidd_StdPixFmt_BGRA32  ; morphfmt_hidd = vHidd_StdPixFmt_BGR032; break;
	case RECTFMT_RGBA32 : dstfmt_hidd = vHidd_StdPixFmt_RGBA32  ; morphfmt_hidd = vHidd_StdPixFmt_RGB032; break;
	case RECTFMT_ABGR32 : dstfmt_hidd = vHidd_StdPixFmt_ABGR32  ; morphfmt_hidd = vHidd_StdPixFmt_0BGR32; break;
	case RECTFMT_RAW  : dstfmt_hidd = vHidd_StdPixFmt_Native; break;
    }

#warning Get rid of the below code ?
/* This can be done by passing the srcx and srcy parameters on to
   the HIDD bitmap and let it take care of it itself.
   This means that HIDD_BM_PutImage() gets a lot of parameters,
   which may not be necessary in real life.
   
   Compromise: convert from *CyberGfx* pixfmt to bppix using a table lookup.
   This is faster
*/
    if ((dstfmt_hidd == vHidd_StdPixFmt_Native) || (morphfmt_hidd != 0))
    {
    	OOP_GetAttr(HIDD_BM_OBJ(rp->BitMap), aHidd_BitMap_PixFmt, (IPTR *)&pf);
    }
    
    if (dstfmt_hidd != vHidd_StdPixFmt_Native)
    {
    	/* RECTFMT_ARGB32 on vHidd_StdPixFmt_0RGB32 bitmap ==> use vHidd_StdPixFmt_0RGB32 */
    	/* RECTFMT_BGRA32 on vHidd_StdPixFmt_BGR032 bitmap ==> use vHidd_StdPixFmt_BGR032 */
    	/* RECTFMT_RGBA32 on vHidd_StdPixFmt_RGB032 bitmap ==> use vHidd_StdPixFmt_RGB032 */
    	/* RECTFMT_ABGR32 on vHidd_StdPixFmt_0BGR32 bitmap ==> use vHidd_StdPixFmt_0BGR32 */
	OOP_Object *gfxhidd;
	
	if (morphfmt_hidd != 0)
	{
	    IPTR stdpf;

	    OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, (IPTR *)&stdpf);	    
	    if (stdpf == morphfmt_hidd) dstfmt_hidd = morphfmt_hidd;
        }

        OOP_GetAttr(HIDD_BM_OBJ(rp->BitMap), aHidd_BitMap_GfxHidd, (IPTR *)&gfxhidd);
    	pf = HIDD_Gfx_GetPixFmt(gfxhidd, dstfmt_hidd);
    }
    
    OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, &bppix);
    
    start_offset = ((ULONG)desty) * dstmod + destx * bppix;
        
    rpard.array	 = ((UBYTE *)dst) + start_offset;
    rpard.pixfmt = dstfmt_hidd;
    rpard.modulo = dstmod;
    rpard.bppix	 = bppix;
    
    rr.MinX = srcx;
    rr.MinY = srcy;
    rr.MaxX = srcx + width  - 1;
    rr.MaxY = srcy + height - 1;
    
    pixread = DoRenderFunc(rp, NULL, &rr, rpa_render, &rpard, FALSE);

    /* restore old drawmode */
    SetDrMd(rp, old_drmd);

    return pixread;
}

LONG driver_InvertPixelArray(struct RastPort *rp
	, UWORD destx, UWORD desty, UWORD width, UWORD height
	, struct IntCGFXBase *CyberGfxBase)
{

    /* This is cybergraphx. We only work wih HIDD bitmaps */
    if (!IS_HIDD_BM(rp->BitMap))
    {
    	D(bug("!!!!! Trying to use CGFX call on non-hidd bitmap InvertPixelArray() !!!\n"));
    	return 0;
    }

    return (LONG)FillRectPenDrMd(rp
   	 , destx, desty
	 , destx + width  - 1
	 , desty + height - 1
	 , 0xFF
	 , vHidd_GC_DrawMode_Invert
         , TRUE);
}

LONG driver_FillPixelArray(struct RastPort *rp
	, UWORD destx, UWORD desty, UWORD width, UWORD height
	, ULONG pixel, struct IntCGFXBase *CyberGfxBase) 
{
    HIDDT_Color col;
    HIDDT_Pixel pix;
    
    /* HIDDT_ColComp are 16 Bit */
    col.alpha	= (HIDDT_ColComp)((pixel >> 16) & 0x0000FF00);
    col.red	= (HIDDT_ColComp)((pixel >> 8) & 0x0000FF00);
    col.green	= (HIDDT_ColComp)(pixel & 0x0000FF00);
    col.blue	= (HIDDT_ColComp)((pixel << 8) & 0x0000FF00);
    
    pix = HIDD_BM_MapColor(HIDD_BM_OBJ(rp->BitMap), &col);

    return (LONG)FillRectPenDrMd(rp
	, destx, desty
	, destx + width  - 1
	, desty + height - 1
	, pix
	, vHidd_GC_DrawMode_Copy
        , TRUE
    );
}

ULONG driver_MovePixelArray(UWORD srcx, UWORD srcy, struct RastPort *rp
	, UWORD destx, UWORD desty, UWORD width, UWORD height
	, struct IntCGFXBase *CyberGfxBase)
{
    ClipBlit(rp
		, srcx, srcy
		, rp
		, destx, desty
		, width, height
		, 0x00C0 /* Copy */
    );
    return width * height;
}



LONG driver_WriteRGBPixel(struct RastPort *rp, UWORD x, UWORD y
	, ULONG pixel, struct IntCGFXBase *CyberGfxBase)
{
    
    struct rgbpix_render_data  prd;
    HIDDT_Color     	    col;
    LONG    	    	    retval;
    
    /* This is cybergraphx. We only work wih HIDD bitmaps */
    if (!IS_HIDD_BM(rp->BitMap))
    {
    	D(bug("!!!!! Trying to use CGFX call on non-hidd bitmap in WriteRGBPixel() !!!\n"));
    	return 0;
    }

    /* HIDDT_ColComp are 16 Bit */
    
    col.alpha	= (HIDDT_ColComp)((pixel >> 16) & 0x0000FF00);
    col.red	= (HIDDT_ColComp)((pixel >> 8) & 0x0000FF00);
    col.green	= (HIDDT_ColComp)(pixel & 0x0000FF00);
    col.blue	= (HIDDT_ColComp)((pixel << 8) & 0x0000FF00);
    
    prd.pixel = HIDD_BM_MapColor(HIDD_BM_OBJ(rp->BitMap), &col);
    
    retval = DoPixelFunc(rp, x, y, rgbpix_write, &prd, TRUE);

    return retval;
   
}


ULONG driver_ReadRGBPixel(struct RastPort *rp, UWORD x, UWORD y
	, struct IntCGFXBase *CyberGfxBase)
{
    struct rgbpix_render_data prd;
    
    /* Get the HIDD pixel val */
    HIDDT_Color col;
    HIDDT_Pixel pix;
    LONG ret;
    
    /* This is cybergraphx. We only work wih HIDD bitmaps */
    if (!IS_HIDD_BM(rp->BitMap))
    {
    	D(bug("!!!!! Trying to use CGFX call on non-hidd bitmap in ReadRGBPixel()!!!\n"));
    	return (ULONG)-1;
    }
    
    ret = DoPixelFunc(rp, x, y, pix_read, &prd, FALSE);
    
    if (-1 == ret)
    	return (ULONG)-1;

    HIDD_BM_UnmapPixel(HIDD_BM_OBJ(rp->BitMap), prd.pixel, &col);

    /* HIDDT_ColComp are 16 Bit */
    
    pix =	  ((col.alpha & 0xFF00) << 16)
    		| ((col.red & 0xFF00) << 8)
		| (col.green & 0xFF00)
		| ((col.blue & 0xFF00) >> 8);
    
    return pix;
}

ULONG driver_ExtractColor(struct RastPort *rp, struct BitMap *bm
	, ULONG color, ULONG srcx, ULONG srcy, ULONG width, ULONG height
	, struct IntCGFXBase *CyberGfxBase)
{
    struct Rectangle rr;
    LONG pixread = 0;
    struct extcol_render_data ecrd;
    OOP_Object *pf;
    IPTR colmod;
    
    if (!IS_HIDD_BM(rp->BitMap))
    {
    	D(bug("!!! CALLING ExtractColor() ON NO-HIDD BITMAP !!!\n"));
	return FALSE;
    }

    rr.MinX = srcx;
    rr.MinY = srcy;
    rr.MaxX = srcx + width  - 1;
    rr.MaxY = srcy + height - 1;
    
    OOP_GetAttr(HIDD_BM_OBJ(rp->BitMap), aHidd_BitMap_PixFmt, (IPTR *)&pf);
    
    OOP_GetAttr(pf, aHidd_PixFmt_ColorModel, (IPTR *)&colmod);
    
    if (vHidd_ColorModel_Palette == colmod)
    {
        ecrd.pixel = color;
    }
    else
    {
	HIDDT_Color col;
	
	col.alpha = (color >> 16) & 0x0000FF00;
	col.red	  = (color >> 8 ) & 0x0000FF00;
	col.green = color & 0x0000FF00;
	col.blue  = (color << 8) & 0x0000FF00;
	
	ecrd.pixel = HIDD_BM_MapColor(HIDD_BM_OBJ(rp->BitMap), &col);
    
    }
    
    ecrd.curbm  = rp->BitMap;
    ecrd.destbm = bm;
    ecrd.CyberGfxBase = CyberGfxBase;
    
    pixread = DoRenderFunc(rp, NULL, &rr, extcol_render, &ecrd, FALSE);
    
    if (pixread != (width * height))
    	return FALSE;
	
    return TRUE;
}



VOID driver_DoCDrawMethodTagList(struct Hook *hook, struct RastPort *rp, struct TagItem *tags, struct IntCGFXBase *CyberGfxBase)
{

    struct dm_render_data dmrd;
    struct Rectangle rr;
    struct Layer *L;
    
    if (!IS_HIDD_BM(rp->BitMap))
    {
    	D(bug("!!! NO HIDD BITMAP IN CALL TO DoCDrawMethodTagList() !!!\n"));
	return;
    }

    /* Get the bitmap std pixfmt */    
    OOP_GetAttr(HIDD_BM_OBJ(rp->BitMap), aHidd_BitMap_PixFmt, (IPTR *)&dmrd.pf);
    OOP_GetAttr(dmrd.pf, aHidd_PixFmt_StdPixFmt, &dmrd.stdpf);
    dmrd.msg.colormodel = hidd2cyber_pixfmt(dmrd.stdpf);
    dmrd.hook = hook;
    dmrd.rp = rp;
    
    if (((UWORD)-1) == dmrd.msg.colormodel)
    {
    	D(bug("!!! UNKNOWN HIDD PIXFMT IN DoCDrawMethodTagList() !!!\n"));
	return;
    }
    
    
    L = rp->Layer;

    rr.MinX = 0;
    rr.MinY = 0;
    
    if (NULL == L)
    {
	rr.MaxX = GetBitMapAttr(rp->BitMap, BMA_WIDTH)  - 1;
	rr.MaxY = GetBitMapAttr(rp->BitMap, BMA_HEIGHT) - 1;
    }
    else
    {
    	/* Lock the layer */
	LockLayerRom(L);
    
    	rr.MaxX = rr.MinX + (L->bounds.MaxX - L->bounds.MinX) - 1;
	rr.MaxY = rr.MinY + (L->bounds.MaxY - L->bounds.MinY) - 1;
    }

    dmrd.CyberGfxBase = GetCGFXBase(CyberGfxBase);
    DoRenderFunc(rp, NULL, &rr, dm_render, &dmrd, TRUE);

    if (NULL != L)
    {
	UnlockLayerRom(L);
    }
    
    return;
}

void driver_BltTemplateAlpha(UBYTE *src, LONG srcx, LONG srcmod
    	, struct RastPort *rp, LONG destx, LONG desty, LONG width, LONG height
	, struct IntCGFXBase *CyberGfxBase)
{
    struct bta_render_data  btard;
    struct Rectangle 	    rr;

    /* This is cybergraphx. We only work wih HIDD bitmaps */
    if (!IS_HIDD_BM(rp->BitMap)) {
    	D(bug("!!!!! Trying to use CGFX call on non-hidd bitmap in BltTemplateAlpha() !!!\n"));
    	return;
    }

    /* Compute the start of the array */

    btard.array  = src + srcx;
    btard.modulo = srcmod;
    btard.invertalpha = (rp->DrawMode & INVERSVID) ? TRUE : FALSE;
    rr.MinX = destx;
    rr.MinY = desty;
    rr.MaxX = destx + width  - 1;
    rr.MaxY = desty + height - 1;
    
    DoRenderFunc(rp, NULL, &rr, bta_render, &btard, TRUE);
}
