/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <hidd/graphics.h>
#include <aros/debug.h>

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

	AROS_LH10(ULONG, ReadPixelArray,

/*  SYNOPSIS */
	AROS_LHA(APTR             , dst		, A0),
	AROS_LHA(UWORD            , destx	, D0),
	AROS_LHA(UWORD            , desty	, D1),
	AROS_LHA(UWORD            , dstmod	, D2),
	AROS_LHA(struct RastPort *, rp		, A1),
	AROS_LHA(UWORD            , srcx	, D3),
	AROS_LHA(UWORD            , srcy	, D4),
	AROS_LHA(UWORD            , width	, D5),
	AROS_LHA(UWORD            , height	, D6),
	AROS_LHA(UBYTE            , dstformat	, D7),

/*  LOCATION */
	struct Library *, CyberGfxBase, 20, Cybergraphics)

/*  FUNCTION
        Copies a rectangular portion of a RastPort to a block of raw pixel
        values.

    INPUTS
        dst - pointer to the pixel values.
        destx, desty - top-lefthand corner of portion of destination rectangle
            to write to (in pixels).
        dstmod - the number of bytes in each row of the destination rectangle.
        rp - the RastPort to read.
        srcx, srcy - top-lefthand corner of portion of source RastPort to
            read (in pixels).
        width, height - size of the area to copy (in pixels).
        dstformat - the format of the destination pixels. The following format
            types are supported:
                RECTFMT_RGB
                RECTFMT_RGBA
                RECTFMT_ARGB
                RECTFMT_RAW
                RECTFMT_RGB15
                RECTFMT_BGR15
                RECTFMT_RGB15PC
                RECTFMT_BGR15PC
                RECTFMT_RGB16
                RECTFMT_BGR16
                RECTFMT_RGB16PC
                RECTFMT_BGR16PC
                RECTFMT_RGB24
                RECTFMT_BGR24
                RECTFMT_0RGB32
                RECTFMT_BGR032
                RECTFMT_RGB032
                RECTFMT_0BGR32
                RECTFMT_ARGB32
                RECTFMT_BGRA32
                RECTFMT_RGBA32
                RECTFMT_ABGR32

    RESULT
        count - number of pixels read.

    NOTES
        See WritePixelArray() for descriptions of pixel formats. Where a
        RastPort does not support an alpha channel, destination alpha values
        will be set to zero.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG start_offset;
    IPTR bppix;
    LONG pixread = 0;
    BYTE old_drmd;
    struct Rectangle rr;
    struct render_data data;

    if (width == 0 || height == 0)
        return 0;

    /* Filter out unsupported modes */
    if (dstformat == RECTFMT_LUT8 || dstformat == RECTFMT_GREY8)
    {
        D(bug("RECTFMT_LUT8 and RECTFMT_GREY8 are not supported "
            "by ReadPixelArray()\n"));
        return 0;
    }

    /* This is cybergraphx. We only work wih HIDD bitmaps */
    if (!IS_HIDD_BM(rp->BitMap))
    {
    	D(bug("!!!!! Trying to use CGFX call on non-hidd bitmap "
            "in ReadPixelArray() !!!\n"));
    	return 0;
    }

    /* Preserve old drawmode */
    old_drmd = rp->DrawMode;
    SetDrMd(rp, JAM2);

    bppix = GetRectFmtBytesPerPixel(dstformat, rp, CyberGfxBase);
    start_offset = ((ULONG)desty) * dstmod + destx * bppix;
    data.array = ((UBYTE *)dst) + start_offset;
    data.pixfmt = GetHIDDRectFmt(dstformat, rp, CyberGfxBase);
    data.modulo = dstmod;
    data.bppix = bppix;

    rr.MinX = srcx;
    rr.MinY = srcy;
    rr.MaxX = srcx + width  - 1;
    rr.MaxY = srcy + height - 1;

    pixread = DoRenderFunc(rp, NULL, &rr, RenderHook, &data, FALSE);

    /* restore old drawmode */
    SetDrMd(rp, old_drmd);

    return pixread;

    AROS_LIBFUNC_EXIT
} /* ReadPixelArray */

static ULONG RenderHook(struct render_data *data, LONG srcx, LONG srcy,
			OOP_Object *dstbm_obj, OOP_Object *dst_gc,
			struct Rectangle *rect, struct GfxBase *GfxBase)
{
    ULONG  width  = rect->MaxX - rect->MinX + 1;
    ULONG  height = rect->MaxY - rect->MinY + 1;
    UBYTE *array = data->array + data->modulo * srcy + data->bppix * srcx;

    HIDD_BM_GetImage(dstbm_obj, array, data->modulo,
    		     rect->MinX, rect->MinY, width, height, data->pixfmt);

    return width * height;
}

