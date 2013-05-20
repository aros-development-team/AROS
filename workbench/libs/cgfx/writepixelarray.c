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

struct render_data
{
    UBYTE *array;
    HIDDT_StdPixFmt pixfmt;
    ULONG modulo;
    ULONG bppix;
};

static ULONG RenderHook(struct render_data *data, LONG srcx, LONG srcy,
    OOP_Object *dstbm_obj, OOP_Object *dst_gc, struct Rectangle *rect,
    struct GfxBase *GfxBase);

/*****************************************************************************

    NAME */
#include <proto/cybergraphics.h>

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

    ULONG start_offset;
    IPTR bppix;
    struct render_data data;
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

    bppix = GetRectFmtBytesPerPixel(srcformat, rp, CyberGfxBase);
    start_offset = ((ULONG)srcy) * srcmod + srcx * bppix;
    data.array = ((UBYTE *)src) + start_offset;
    data.pixfmt = GetHIDDRectFmt(srcformat, rp, CyberGfxBase);
    data.modulo = srcmod;
    data.bppix = bppix;
    
    rr.MinX = destx;
    rr.MinY = desty;
    rr.MaxX = destx + width  - 1;
    rr.MaxY = desty + height - 1;

    return DoRenderFunc(rp, NULL, &rr, RenderHook, &data, TRUE);

    AROS_LIBFUNC_EXIT
} /* WritePixelArray */

static ULONG RenderHook(struct render_data *data, LONG srcx, LONG srcy,
    OOP_Object *dstbm_obj, OOP_Object *dst_gc, struct Rectangle *rect,
    struct GfxBase *GfxBase)
{
    ULONG  width  = rect->MaxX - rect->MinX + 1;
    ULONG  height = rect->MaxY - rect->MinY + 1;
    UBYTE *array  = data->array + data->modulo * srcy + data->bppix * srcx;

    HIDD_BM_PutImage(dstbm_obj, dst_gc, array, data->modulo,
    		     rect->MinX, rect->MinY, width, height, data->pixfmt);

    return width * height;
}

