/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/debug.h>
#include <proto/graphics.h>
#include <proto/oop.h>
#include "graphics_intern.h"
#include "gfxfuncsupport.h"
#include <hardware/blit.h>

struct bltmask_render_data
{
    struct render_special_info rsi;
    struct BitMap   	       *srcbm;
    OOP_Object      	       *srcbm_obj;
    OOP_Object	    	       *srcpf;
    HIDDT_ColorModel	       src_colmod;
    PLANEPTR	    	       mask;
    ULONG   	    	       mask_bpr;
    ULONG   	    	       minterm;
    BOOL    	    	       samebitmapformat;
};

static ULONG bltmask_render(APTR bltmask_rd, LONG srcx, LONG srcy,
    	    	    	    OOP_Object *dstbm_obj, OOP_Object *dst_gc,
			    LONG x1, LONG y1, LONG x2, LONG y2, struct GfxBase *GfxBase);

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH10(void, BltMaskBitMapRastPort,

/*  SYNOPSIS */
	AROS_LHA(struct BitMap   *, srcBitMap, A0),
	AROS_LHA(LONG             , xSrc, D0),
	AROS_LHA(LONG             , ySrc, D1),
	AROS_LHA(struct RastPort *, destRP, A1),
	AROS_LHA(LONG             , xDest, D2),
	AROS_LHA(LONG             , yDest, D3),
	AROS_LHA(LONG             , xSize, D4),
	AROS_LHA(LONG             , ySize, D5),
	AROS_LHA(ULONG            , minterm, D6),
	AROS_LHA(PLANEPTR         , bltMask, A2),

/*  LOCATION */
	struct GfxBase *, GfxBase, 106, Graphics)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)
    
    struct bltmask_render_data 	brd;
    struct Rectangle 	    	rr;
    HIDDT_DrawMode  	    	old_drmd;
    OOP_Object      	    	*gc;
    Point   	    	    	src;

    struct TagItem gc_tags[] =
    {
    	{ aHidd_GC_DrawMode , 0UL },
	{ TAG_DONE  	    	  }
    };

    EnterFunc(bug("BltMaskBitMapRastPort(%d %d %d, %d, %d, %d)\n"
    	, xSrc, ySrc, xDest, yDest, xSize, ySize));

    FIX_GFXCOORD(xSrc);
    FIX_GFXCOORD(ySrc);
    FIX_GFXCOORD(xDest);
    FIX_GFXCOORD(yDest);
    
    if ((xSize < 1) || (ySize < 1)) return;
    
    if (!OBTAIN_DRIVERDATA(destRP, GfxBase))
    	return;

    brd.minterm	= minterm;
    brd.srcbm_obj = OBTAIN_HIDD_BM(srcBitMap);
    if (NULL == brd.srcbm_obj)
    {
    	RELEASE_DRIVERDATA(destRP, GfxBase);
    	return;
    }
    
    brd.srcbm = srcBitMap;
    brd.mask  = bltMask;

    /* The mask has always the same size as the source bitmap */    
    brd.mask_bpr = 2 * WIDTH_TO_WORDS(GetBitMapAttr(srcBitMap, BMA_WIDTH));

    OOP_GetAttr(brd.srcbm_obj, aHidd_BitMap_PixFmt, (IPTR *)&brd.srcpf);
    {
    	IPTR attr;
	
	OOP_GetAttr(brd.srcpf, aHidd_PixFmt_ColorModel, &attr);
	brd.src_colmod = (HIDDT_ColorModel)attr;
    }
    
    gc = GetDriverData(destRP)->dd_GC;
    OOP_GetAttr(gc, aHidd_GC_DrawMode, &old_drmd);

    gc_tags[0].ti_Data = vHidd_GC_DrawMode_Copy;
    OOP_SetAttrs(gc, gc_tags);

    rr.MinX = xDest;
    rr.MinY = yDest;
    rr.MaxX = xDest + xSize - 1;
    rr.MaxY = yDest + ySize - 1;

    src.x = xSrc;
    src.y = ySrc;

    do_render_func(destRP, &src, &rr, bltmask_render, &brd, TRUE, GfxBase);

    RELEASE_HIDD_BM(brd.srcbm_obj, srcBitMap);

    gc_tags[0].ti_Data = old_drmd;
    OOP_SetAttrs(gc, gc_tags);

    RELEASE_DRIVERDATA(destRP, GfxBase);
    
    ReturnVoid("BltBitMapRastPort");
    
    AROS_LIBFUNC_EXIT
    
} /* BltMaskBitMapRastPort */

/****************************************************************************************/

static ULONG bltmask_render(APTR bltmask_rd, LONG srcx, LONG srcy,
    	    	    	    OOP_Object *dstbm_obj, OOP_Object *dst_gc,
			    LONG x1, LONG y1, LONG x2, LONG y2, struct GfxBase *GfxBase)
{
    struct bltmask_render_data 	*brd;
    OOP_Object	    	    	*dest_pf;
    HIDDT_ColorModel	    	dest_colmod;
    HIDDT_Pixel     	    	*pixtab = NULL;
    ULONG   	    	    	x, y, width, height;
    LONG    	    	    	lines_done, lines_per_step, doing_lines;
    BOOL    	    	    	pal_to_true = FALSE;
    
    width  = x2 - x1 + 1;
    height = y2 - y1 + 1;

    OOP_GetAttr(dstbm_obj, aHidd_BitMap_PixFmt, (IPTR *)&dest_pf);
    {
    	IPTR attr;
	
    	OOP_GetAttr(dest_pf, aHidd_PixFmt_ColorModel, &attr);
	dest_colmod = (HIDDT_ColorModel)attr;
    }

    brd = (struct bltmask_render_data *)bltmask_rd;
    if ((brd->src_colmod == vHidd_ColorModel_Palette) && (dest_colmod == vHidd_ColorModel_Palette))
    {
    }
    else if ((brd->src_colmod == vHidd_ColorModel_Palette) && (dest_colmod == vHidd_ColorModel_TrueColor))
    {
    	pixtab = IS_HIDD_BM(brd->srcbm) ? HIDD_BM_PIXTAB(brd->srcbm) : NULL;
	if (!pixtab) pixtab = IS_HIDD_BM(brd->rsi.curbm) ? HIDD_BM_PIXTAB(brd->rsi.curbm) : NULL;
	
	if (!pixtab)
	{
	    D(bug("BltMaskBitMapRastPort could not retrieve pixel table for blit from palette to truecolor bitmap"));
	    return width * height;
	}
	
    	pal_to_true = TRUE;
    }
    else if ((brd->src_colmod == vHidd_ColorModel_TrueColor) && (dest_colmod == vHidd_ColorModel_TrueColor))
    {
    	if (brd->srcpf != dest_pf)
	{
	    D(bug("BltMaskBitMapRastPort not supporting blit between truecolor bitmaps of different formats yet"));
	    return width * height;
	}
    }
    else if ((brd->src_colmod == vHidd_ColorModel_TrueColor) && (dest_colmod == vHidd_ColorModel_Palette))
    {
    	D(bug("BltMaskBitMapRastPort from truecolor bitmap to palette bitmap not supported!"));
    	return width * height;
    }

    lines_per_step = (NUMPIX / (width * 2 * sizeof(HIDDT_Pixel)));
    if (lines_per_step)
    {
    	UBYTE *srcbuf, *destbuf;
	
	LOCK_PIXBUF

    	srcbuf = (UBYTE *)PrivGBase(GfxBase)->pixel_buf;
	destbuf = srcbuf;
	destbuf += lines_per_step * width * sizeof(HIDDT_Pixel);
	
	for(lines_done = 0; lines_done != height; lines_done += doing_lines)
	{
	    HIDDT_Pixel *srcpixelbuf;
	    HIDDT_Pixel *destpixelbuf;
	    UBYTE   	*mask;
	    
	    doing_lines = lines_per_step;
	    if (lines_done + doing_lines > height) doing_lines = height - lines_done;
	    
	    HIDD_BM_GetImage(brd->srcbm_obj,
	    	    	     srcbuf,
			     width * sizeof(HIDDT_Pixel),
			     srcx, srcy + lines_done,
			     width, doing_lines,
			     vHidd_StdPixFmt_Native32);
			     
	    HIDD_BM_GetImage(dstbm_obj,
	    	    	     destbuf,
			     width * sizeof(HIDDT_Pixel),
			     x1, y1 + lines_done,
			     width, doing_lines,
			     vHidd_StdPixFmt_Native32);
		
	    mask = &brd->mask[COORD_TO_BYTEIDX(0, srcy + lines_done, brd->mask_bpr)];
	    
	    srcpixelbuf  = (HIDDT_Pixel *)srcbuf;
	    destpixelbuf = (HIDDT_Pixel *)destbuf;
	    
	    switch(brd->minterm)
	    {
	    	case (ABC|ABNC|ANBC): /* (ABC|ABNC|ANBC) if copy source and blit thru mask */
		    if (pal_to_true)
		    {
		    	for(y = 0; y < doing_lines; y++)
			{
		    	    for(x = 0; x < width; x++)
			    {
			    	if (mask[XCOORD_TO_BYTEIDX(srcx + x)] & XCOORD_TO_MASK(srcx + x))
				{
				    *destpixelbuf = pixtab[*srcpixelbuf];
				}
				srcpixelbuf++;
				destpixelbuf++;
			    }
			    mask += brd->mask_bpr;
			}
		    }
		    else
		    {
		    	for(y = 0; y < doing_lines; y++)
			{
		    	    for(x = 0; x < width; x++)
			    {
			    	if (mask[XCOORD_TO_BYTEIDX(srcx + x)] & XCOORD_TO_MASK(srcx + x))
				{
				    *destpixelbuf = *srcpixelbuf;
				}
				srcpixelbuf++;
				destpixelbuf++;
			    }
			    mask += brd->mask_bpr;
			}
		    }
		    break;
		    
		case ANBC: /* (ANBC) if invert source and blit thru mask */
		    D(bug("BltMaskBitMapRastPort does not support ANBC minterm yet"));
		    break;

		default:
		    D(bug("BltMaskBitMapRastPort: minterm 0x%x not handled.\n", brd->minterm));
		    break;
	    } /* switch(brd->minterm) */
	    
	    HIDD_BM_PutImage(dstbm_obj,
	    	    	     dst_gc,
	    	    	     destbuf,
			     width * sizeof(HIDDT_Pixel),
			     x1, y1 + lines_done,
			     width, doing_lines,
			     vHidd_StdPixFmt_Native32);
			     
	} /* for(lines_done = 0; lines_done != height; lines_done += doing_lines) */

	ULOCK_PIXBUF
	
    } /* if (lines_per_step) */
    else
    {
    	/* urk :-( pixelbuffer too small to hold two lines) */
	
	D(bug("BltMaskBitMapRastPort found pixelbuffer to be too small"));
    }
    
    return width * height;
}

/****************************************************************************************/
