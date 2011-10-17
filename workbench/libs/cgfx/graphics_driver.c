/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

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

struct wpaa_render_data
{
    UBYTE *array;
    ULONG modulo;
};

static ULONG wpaa_render(struct wpaa_render_data *wpaard, LONG srcx, LONG srcy,
			 OOP_Object *dstbm_obj, OOP_Object *dst_gc,
			 struct Rectangle *rect, struct GfxBase *GfxBase)
{
    ULONG  width  = rect->MaxX - rect->MinX + 1;
    ULONG  height = rect->MaxY - rect->MinY + 1;
    UBYTE *array = wpaard->array + wpaard->modulo * srcy + 4 * srcx;

    HIDD_BM_PutAlphaImage(dstbm_obj, dst_gc, array, wpaard->modulo,
    			  rect->MinX, rect->MinY, width, height);

    return width * height;
}

struct bta_render_data
{
    UBYTE *array;
    ULONG  modulo;
    UBYTE  invertalpha;
};

static ULONG bta_render(struct bta_render_data *btard, LONG srcx, LONG srcy,
			OOP_Object *dstbm_obj, OOP_Object *dst_gc,
			struct Rectangle *rect, struct GfxBase *GfxBase)
{
    ULONG  width  = rect->MaxX - rect->MinX + 1;
    ULONG  height = rect->MaxY - rect->MinY + 1;
    UBYTE *array = btard->array + btard->modulo * srcy + srcx;

    HIDD_BM_PutAlphaTemplate(dstbm_obj, dst_gc, array, btard->modulo,
			     rect->MinX, rect->MinY, width, height, btard->invertalpha);

    return width * height;
}

struct rpa_render_data {
    UBYTE *array;
    HIDDT_StdPixFmt pixfmt;
    ULONG modulo;
    ULONG bppix;
};

static ULONG rpa_render(struct rpa_render_data *rpard, LONG srcx, LONG srcy,
			OOP_Object *dstbm_obj, OOP_Object *dst_gc,
			struct Rectangle *rect, struct GfxBase *GfxBase)
{
    ULONG  width  = rect->MaxX - rect->MinX + 1;
    ULONG  height = rect->MaxY - rect->MinY + 1;
    UBYTE *array = rpard->array + rpard->modulo * srcy + rpard->bppix * srcx;

    HIDD_BM_GetImage(dstbm_obj, array, rpard->modulo,
    		     rect->MinX, rect->MinY, width, height, rpard->pixfmt);
    
    return width * height;
}

static LONG pix_read(struct rgbpix_render_data *prd, OOP_Object *bm, OOP_Object *gc,
		     LONG x, LONG y, struct GfxBase *GfxBase)
{   
    prd->pixel = HIDD_BM_GetPixel(bm, x, y);
    return 0;
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


static ULONG dm_render(struct dm_render_data *dmrd, LONG srcx, LONG srcy,
		       OOP_Object *dstbm_obj, OOP_Object *dst_gc,
		       struct Rectangle *rect, struct GfxBase *GfxBase)
{
    struct IntCGFXBase *CyberGfxBase = dmrd->CyberGfxBase;
    struct dm_message *msg = &dmrd->msg;
    ULONG  width  = rect->MaxX - rect->MinX + 1;
    ULONG  height = rect->MaxY - rect->MinY + 1;
    UBYTE *addr;
    IPTR   bytesperpixel, bytesperrow;
    ULONG  fb_width, fb_height;
    ULONG  banksize, memsize;

#if 1
    msg->offsetx = rect->MinX;
    msg->offsety = rect->MinY;
#else
    #warning "Not sure about this one . Set it to 0 since we adjust msg->memptr to x1/y1 lower down"
    msg->offsetx = 0; // x1;
    msg->offsety = 0; // y1;
#endif
    msg->xsize = width;
    msg->ysize = height;

    OOP_GetAttr(dmrd->pf, aHidd_PixFmt_BytesPerPixel, &bytesperpixel);
    OOP_GetAttr(dstbm_obj, aHidd_BitMap_BytesPerRow, &bytesperrow);

    D(kprintf("width %d bytesperrow %d bytesperpixel %d\n", width, bytesperrow, bytesperpixel));
    D(kprintf(" colormodel %d\n", msg->colormodel));

    /* Get the baseadress from where to render */
    if (HIDD_BM_ObtainDirectAccess(dstbm_obj, &addr, &fb_height, &fb_width, &banksize, &memsize))
    {
	msg->bytesperpix = (UWORD)bytesperpixel;
	msg->bytesperrow = bytesperrow;
	/* Colormodel allready set */

	/* Compute the adress for the start pixel */
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
	ULONG tocopy_h, max_tocopy_h;
	ULONG lines_todo;
	OOP_Object *gfxhidd, *gc;
    
	lines_todo = height;
    
	/* The HIDD bm does not have a base adress so we have to render into
	   it using a temporary buffer
	*/

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
	    HIDD_BM_GetImage(dstbm_obj, (UBYTE *)CyberGfxBase->pixel_buf, bytesperrow,
			     rect->MinX, rect->MinY + height - lines_todo, width, lines_todo, dmrd->stdpf);

	    /* Use the hook to set some pixels */
	    CallHookPkt(dmrd->hook, dmrd->rp, msg);

	    HIDD_BM_PutImage(dstbm_obj, gc, (UBYTE *)CyberGfxBase->pixel_buf, bytesperrow,
	    		     rect->MinX, rect->MinY + height - lines_todo, width, lines_todo, dmrd->stdpf);

ULOCK_PIXBUF

    	    lines_todo -= tocopy_h;
	}

	OOP_DisposeObject(gc);
    }
    
    return width * height;
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
    
    /* Filter out unsupported modes */
    if ((dstformat == RECTFMT_LUT8) || (dstformat == RECTFMT_GREY8))
    {
        D(bug("RECTFMT_LUT8 and RECTFMT_GREY8 are not supported by ReadPixelArray()\n"));
        return 0;
    }

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

/* FIXME: Get rid of the below code ? */
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
    dmrd.msg.colormodel = hidd2cyber_pixfmt[dmrd.stdpf];
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
