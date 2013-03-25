/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/debug.h>
#include <hidd/graphics.h>
#include <proto/cybergraphics.h>

#include "cybergraphics_intern.h"
#include "gfxfuncsupport.h"

struct wpa_render_data
{
    UBYTE *array;
    HIDDT_StdPixFmt pixfmt;
    ULONG modulo;
    ULONG bppix;
};

static ULONG wpa_render(struct wpa_render_data *wpard, LONG srcx, LONG srcy,
			OOP_Object *dstbm_obj, OOP_Object *dst_gc,
			struct Rectangle *rect, struct GfxBase *GfxBase)
{
    ULONG  width  = rect->MaxX - rect->MinX + 1;
    ULONG  height = rect->MaxY - rect->MinY + 1;
    UBYTE *array  = wpard->array + wpard->modulo * srcy + wpard->bppix * srcx;

    HIDD_BM_PutImage(dstbm_obj, dst_gc, array, wpard->modulo,
    		     rect->MinX, rect->MinY, width, height, wpard->pixfmt);

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
        Copies all or part of a rectangular block of raw pixel values to a
        RastPort.

    INPUTS
        srcRect - pointer to the pixel values.
        srcx, srcy - top-lefthand corner of portion of source rectangle to
            copy (in pixels).
        srcmod - the number of bytes in each row of the source rectangle.
        rp - the RastPort to write to.
        destx, desty - top-lefthand corner of portion of destination RastPort
            to write to (in pixels).
        width, height - size of the area to copy (in pixels).
        srcformat - the format of the source pixels. The following format
            types are supported:
                RECTFMT_RGB - 3 bytes per pixel: 1 byte per component, in
                    the order red, green, blue.
                RECTFMT_RGBA - 4 bytes per pixel: 1 byte per component, in
                    the order red, green, blue, alpha.
                RECTFMT_ARGB - 4 bytes per pixel: 1 byte per component, in
                    the order alpha, red, green, blue.
                RECTFMT_LUT8 - 1 byte per pixel: each byte is a pen number
                    rather than a direct colour value.
                RECTFMT_GREY8 - 1 byte per pixel: each byte is a greyscale
                    value.
                RECTFMT_RAW - the same pixel format as the destination
                    RastPort.
                RECTFMT_RGB15 - 2 bytes per pixel: one unused bit, then 5 bits
                    per component, in the order red, green, blue (AROS
                    extension).
                RECTFMT_BGR15 - 2 bytes per pixel: 1 unused bit, then 5 bits
                    per component, in the order blue, green, red (AROS
                    extension).
                RECTFMT_RGB15PC - 2 bytes per pixel, accessed as a little
                    endian value: 1 unused bit, then 5 bits per component, in
                    the order red, green, blue (AROS extension).
                RECTFMT_BGR15PC - 2 bytes per pixel, accessed as a little
                    endian value: 1 unused bit, then 5 bits per component, in
                    the order blue, green, red (AROS extension).
                RECTFMT_RGB16 - 2 bytes per pixel: 5 bits for red, then 6 bits
                    for green, then 5 bits for blue (AROS extension).
                RECTFMT_BGR16 - 2 bytes per pixel: 5 bits for blue, then 6 bits
                    for green, then 5 bits for red (AROS extension).
                RECTFMT_RGB16PC - 2 bytes per pixel, accessed as a little
                    endian value: 5 bits for red, then 6 bits for green, then
                    5 bits for blue (AROS extension).
                RECTFMT_BGR16PC - 2 bytes per pixel, accessed as a little
                    endian value: 5 bits for blue, then 6 bits for green, then
                    5 bits for red (AROS extension).
                RECTFMT_RGB24 - the same as RECTFMT_RGB (AROS extension).
                RECTFMT_BGR24 - 3 bytes per pixel: 1 byte per component, in
                    the order blue, green, red (AROS extension).
                RECTFMT_ARGB32 - the same as RECTFMT_ARGB (AROS extension).
                RECTFMT_BGRA32 - 4 bytes per pixel: 1 byte per component, in
                    the order blue, green, red, alpha (AROS extension).
                RECTFMT_RGBA32 - the same as RECTFMT_RGBA (AROS extension).
                RECTFMT_ABGR32 - 4 bytes per pixel: 1 byte per component, in
                    the order alpha, blue, green, red (AROS extension).
                RECTFMT_0RGB32 - 4 bytes per pixel: 1 unused byte, then 1 byte
                    per component, in the order red, green, blue (AROS
                    extension).
                RECTFMT_BGR032 - 4 bytes per pixel: 1 byte per component, in
                    the order blue, green, red, followed by 1 unused byte
                    (AROS extension).
                RECTFMT_RGB032 - 4 bytes per pixel: 1 byte per component, in
                    the order red, green, blue, followed by 1 unused byte
                    (AROS extension).
                RECTFMT_0BGR32 - 4 bytes per pixel: 1 unused byte, then 1 byte
                    per component, in the order blue, green, red (AROS
                    extension).

    RESULT
        count - the number of pixels written to.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        WritePixelArrayAlpha()

    INTERNALS

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
