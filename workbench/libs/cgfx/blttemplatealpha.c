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

struct render_data
{
    UBYTE *array;
    ULONG modulo;
    UBYTE invertalpha;
};

static ULONG RenderHook(struct render_data *data, LONG srcx, LONG srcy,
    OOP_Object *dstbm_obj, OOP_Object *dst_gc, struct Rectangle *rect,
    struct GfxBase *GfxBase);

/*****************************************************************************

    NAME */
#include <proto/cybergraphics.h>

	AROS_LH8(void, BltTemplateAlpha,

/*  SYNOPSIS */
	AROS_LHA(APTR             , src		, A0),
	AROS_LHA(LONG             , srcx	, D0),
	AROS_LHA(LONG             , srcmod	, D1),
	AROS_LHA(struct RastPort *, rp		, A1),
	AROS_LHA(LONG             , destx	, D2),
	AROS_LHA(LONG             , desty	, D3),
	AROS_LHA(LONG	          , width	, D4),
	AROS_LHA(LONG             , height	, D5),

/*  LOCATION */
	struct Library *, CyberGfxBase, 37, Cybergraphics)

/*  FUNCTION
        Alpha blends the current foreground colour into a rectangular portion
        of a RastPort. The source alpha channel to use for each pixel is taken
        from an array of 8-bit alpha values. This alpha template may be any
        rectangle within a larger array/rectangle of alpha values.

    INPUTS
        src - pointer to an array of source alpha values.
        srcx - byte/pixel offset of top-lefthand corner of alpha template.
        srcmod - the number of bytes in each row of the source array.
        rp - the RastPort to write to.
        destx, desty - top-lefthand corner of portion of destination RastPort
            to write to (in pixels).
        width, height - size of the area to copy (in pixels).

    RESULT
        None.

    NOTES
        The size and destination coordinates may be outside the RastPort
        boundaries, in which case the affected area is safely truncated.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct render_data data;
    struct Rectangle rr;

    if (width == 0 || height == 0)
        return;

    /* This is cybergraphx. We only work wih HIDD bitmaps */
    if (!IS_HIDD_BM(rp->BitMap)) {
    	D(bug("!!!!! Trying to use CGFX call on non-hidd bitmap "
            "in BltTemplateAlpha() !!!\n"));
    	return;
    }

    /* Compute the start of the array */

    data.array  = src + srcx;
    data.modulo = srcmod;
    data.invertalpha = (rp->DrawMode & INVERSVID) ? TRUE : FALSE;
    rr.MinX = destx;
    rr.MinY = desty;
    rr.MaxX = destx + width  - 1;
    rr.MaxY = desty + height - 1;
    
    DoRenderFunc(rp, NULL, &rr, RenderHook, &data, TRUE);

    AROS_LIBFUNC_EXIT
} /* BltTemplateAlpha */

static ULONG RenderHook(struct render_data *data, LONG srcx, LONG srcy,
    OOP_Object *dstbm_obj, OOP_Object *dst_gc, struct Rectangle *rect,
    struct GfxBase *GfxBase)
{
    ULONG  width  = rect->MaxX - rect->MinX + 1;
    ULONG  height = rect->MaxY - rect->MinY + 1;
    UBYTE *array = data->array + data->modulo * srcy + srcx;

    HIDD_BM_PutAlphaTemplate(dstbm_obj, dst_gc, array, data->modulo,
        rect->MinX, rect->MinY, width, height, data->invertalpha);

    return width * height;
}

