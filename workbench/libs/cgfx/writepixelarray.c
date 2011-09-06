/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/debug.h>
#include <hidd/graphics.h>

#include "cybergraphics_intern.h"
#include "gfxfuncsupport.h"

struct wpa_render_data
{
    UBYTE *array;
    HIDDT_StdPixFmt pixfmt;
    ULONG modulo;
    ULONG bppix;
};

static ULONG wpa_render(struct wpa_render_data *wpard
	, LONG srcx, LONG srcy
	, OOP_Object *dstbm_obj
	, OOP_Object *dst_gc
	, LONG x1, LONG y1, LONG x2, LONG y2
	, struct GfxBase *GfxBase)
{
    ULONG width, height;
    UBYTE *array;

    width  = x2 - x1 + 1;
    height = y2 - y1 + 1;

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

/*****************************************************************************

    NAME */
#include <clib/cybergraphics_protos.h>

	AROS_LH10(ULONG, WritePixelArray,

/*  SYNOPSIS */
	AROS_LHA(APTR             , src		, A0),
	AROS_LHA(UWORD            , srcx	, D0),
	AROS_LHA(UWORD            , srcy	, D1),
	AROS_LHA(UWORD            , srcmod	, D2),
	AROS_LHA(struct RastPort *, rp		, A1),
	AROS_LHA(UWORD            , destx	, D3),
	AROS_LHA(UWORD            , desty	, D4),
	AROS_LHA(UWORD            , width	, D5),
	AROS_LHA(UWORD            , height	, D6),
	AROS_LHA(UBYTE            , srcformat	, D7),

/*  LOCATION */
	struct Library *, CyberGfxBase, 21, Cybergraphics)

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
			    cybergraphics_lib.fd and clib/cybergraphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    OOP_Object *pf = 0;
    HIDDT_StdPixFmt srcfmt_hidd = 0, morphfmt_hidd = 0;
    ULONG start_offset;
    IPTR bppix;
    struct wpa_render_data wpard;
    struct Rectangle rr;

    if ((!width) || (!height))
    	return 0;

    if (RECTFMT_GREY8 == srcformat)
    {
	/* This just uses our builtin palette */
	return WriteLUTPixelArray(src, srcx, srcy, srcmod,
	    	    	    	  rp, GetCGFXBase(CyberGfxBase)->greytab,
	    	    	    	  destx, desty, width, height, CTABFMT_XRGB8);
    }

    /* This is cybergraphx. We only work wih HIDD bitmaps */
    if (!IS_HIDD_BM(rp->BitMap))
    {
    	D(bug("!!!!! Trying to use CGFX call on non-hidd bitmap in WritePixelArray() !!!\n"));
    	return 0;
    }
    
    if (RECTFMT_LUT8 == srcformat)
    {
	/* Actually this is the same as WriteChunkyPixels() with return value */
    	return WritePixels8(rp, src, srcmod,
    			    destx, desty,
			    destx + width - 1, desty + height - 1,
			    NULL, TRUE);
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

/* FIXME: Get rid of the below code ? */
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

    return DoRenderFunc(rp, NULL, &rr, wpa_render, &wpard, TRUE);

    AROS_LIBFUNC_EXIT
} /* WritePixelArray */
