/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <hidd/gfx.h>
#include <aros/debug.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include "cybergraphics_intern.h"
#include "gfxfuncsupport.h"

struct render_data
{
    UBYTE *array;
    ULONG modulo;
    UBYTE alpha;    /* global alpha, 0xFF = source alpha only */
};

/* Rows are scaled through a bounded buffer so large blits need no large allocation */
#define WPAA_BUFSIZE 65536

static ULONG RenderHook(struct render_data *data, LONG srcx, LONG srcy,
    OOP_Object *dstbm_obj, OOP_Object *dst_gc, struct Rectangle *rect,
    struct GfxBase *GfxBase);

/*****************************************************************************

    NAME */
#include <proto/cybergraphics.h>

        AROS_LH10(ULONG, WritePixelArrayAlpha,

/*  SYNOPSIS */
        AROS_LHA(APTR             , src         , A0),
        AROS_LHA(UWORD            , srcx        , D0),
        AROS_LHA(UWORD            , srcy        , D1),
        AROS_LHA(UWORD            , srcmod      , D2),
        AROS_LHA(struct RastPort *, rp          , A1),
        AROS_LHA(UWORD            , destx       , D3),
        AROS_LHA(UWORD            , desty       , D4),
        AROS_LHA(UWORD            , width       , D5),
        AROS_LHA(UWORD            , height      , D6),
        AROS_LHA(ULONG            , globalalpha , D7),

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
            from the source rectangle, on top of each pixel's own alpha.
            The full 32-bit range is used: 0xFFFFFFFF leaves the source
            alpha as it is, 0x80000000 halves it, 0 draws nothing. Only the
            most significant 8 bits take part.

    RESULT
        count - the number of pixels written to.

    NOTES
        Because of the X11 driver you have to set the drawmode
        to JAM1 with SetDrMd().

    EXAMPLE

    BUGS

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

    /* Fully transparent: nothing to draw */
    if ((globalalpha >> 24) == 0)
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
    data.alpha  = globalalpha >> 24;

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
    ULONG  modulo, rows, bufsize, y;
    UBYTE *buf;

    if (data->alpha == 0xFF)
    {
        HIDD_BM_PutAlphaImage(dstbm_obj, dst_gc, array, data->modulo,
            rect->MinX, rect->MinY, width, height);

        return width * height;
    }

    /*
     * Apply the global alpha to a copy of the source, a few rows at a time:
     * the caller's array must not be modified. The source is ARGB, alpha
     * being the first byte of each pixel.
     */
    modulo = width * 4;
    rows = WPAA_BUFSIZE / modulo;
    if (rows == 0)
        rows = 1;
    if (rows > height)
        rows = height;
    bufsize = rows * modulo;

    buf = AllocMem(bufsize, MEMF_ANY);
    if (!buf)
    {
        rows = 1;
        bufsize = modulo;
        buf = AllocMem(bufsize, MEMF_ANY);
        if (!buf)
            return 0;
    }

    for (y = 0; y < height; y += rows)
    {
        ULONG  n = height - y;
        ULONG  r, i;

        if (n > rows)
            n = rows;

        for (r = 0; r < n; r++)
        {
            UBYTE *a = buf + r * modulo;

            /* Bytewise: the caller's array need not be longword aligned */
            CopyMem(array + (y + r) * data->modulo, a, modulo);

            for (i = 0; i < width; i++, a += 4)
            {
                ULONG v = *a * data->alpha + 127;

                *a = (v + (v >> 8)) >> 8;
            }
        }

        HIDD_BM_PutAlphaImage(dstbm_obj, dst_gc, buf, modulo,
            rect->MinX, rect->MinY + y, width, n);
    }

    FreeMem(buf, bufsize);

    return width * height;
}

