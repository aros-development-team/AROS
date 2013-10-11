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
    ULONG modulo;
};

static ULONG RenderHook(struct render_data *data, LONG srcx, LONG srcy,
    OOP_Object *dstbm_obj, OOP_Object *dst_gc, struct Rectangle *rect,
    struct GfxBase *GfxBase);

/*****************************************************************************

    NAME */
#include <proto/cybergraphics.h>

	AROS_LH10(ULONG, WritePixelArrayAlpha,

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
	AROS_LHA(ULONG            , globalalpha	, D7),

/*  LOCATION */
	struct Library *, CyberGfxBase, 36, Cybergraphics)

/*  FUNCTION
        Alpha-blends all or part of a rectangular block of raw pixel values
        into a RastPort. The source data must be in 32-bit ARGB format: 1 byte
        per component, in the order alpha, red, green, blue.

    INPUTS
        srcRect - pointer to the pixel values.
        srcx, srcy - top-lefthand corner of portion of source rectangle to
            use (in pixels).
        srcmod - the number of bytes in each row of the source rectangle.
        rp - the RastPort to write to.
        destx, desty - top-lefthand corner of portion of destination RastPort
            to write to (in pixels).
        width, height - size of the affected area (in pixels).
        globalalpha - an alpha value applied globally to every pixel taken
            from the source rectangle (the full 32-bit range of values is
            used: 0 to 0xFFFFFFFF).

    RESULT
        count - the number of pixels written to.

    NOTES
        Because of the X11 driver you have to set the drawmode
        to JAM1 with SetDrMd().

    EXAMPLE

    BUGS
        The globalalpha parameter is currently ignored.

    SEE ALSO
        WritePixelArray(), graphics.library/SetDrMd()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG start_offset;    
    LONG pixwritten = 0;    
    struct render_data data;
    struct Rectangle rr;

    if (width == 0 || height == 0)
        return 0;

    /* This is cybergraphx. We only work wih HIDD bitmaps */
    if (!IS_HIDD_BM(rp->BitMap))
    {
    	D(bug("!!!!! Trying to use CGFX call on non-hidd bitmap "
            "in WritePixelArrayAlpha() !!!\n"));
    	return 0;
    }

    /* Compute the start of the array */

    start_offset = ((ULONG)srcy) * srcmod + srcx * 4;

    data.array  = ((UBYTE *)src) + start_offset;
    data.modulo = srcmod;

    rr.MinX = destx;
    rr.MinY = desty;
    rr.MaxX = destx + width  - 1;
    rr.MaxY = desty + height - 1;

    pixwritten = DoRenderFunc(rp, NULL, &rr, RenderHook, &data, TRUE);

    return pixwritten;

    AROS_LIBFUNC_EXIT
} /* WritePixelArrayAlpha */

static ULONG RenderHook(struct render_data *data, LONG srcx, LONG srcy,
    OOP_Object *dstbm_obj, OOP_Object *dst_gc, struct Rectangle *rect,
    struct GfxBase *GfxBase)
{
    ULONG  width  = rect->MaxX - rect->MinX + 1;
    ULONG  height = rect->MaxY - rect->MinY + 1;
    UBYTE *array = data->array + data->modulo * srcy + 4 * srcx;

    HIDD_BM_PutAlphaImage(dstbm_obj, dst_gc, array, data->modulo,
        rect->MinX, rect->MinY, width, height);

    return width * height;
}

