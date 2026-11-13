/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Blend a rectangle of one bitmap over another one. AROS extension.
*/

#include <aros/debug.h>
#include <graphics/gfx.h>
#include <hidd/gfx.h>
#include <proto/oop.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"
#include "objcache.h"

/*****************************************************************************

    NAME */
#include <graphics/gfx.h>
#include <proto/graphics.h>

        AROS_LH9(void, BltBitMapAlpha,

/*  SYNOPSIS */
        AROS_LHA(struct BitMap *, srcBitMap, A0),
        AROS_LHA(LONG           , xSrc, D0),
        AROS_LHA(LONG           , ySrc, D1),
        AROS_LHA(struct BitMap *, destBitMap, A1),
        AROS_LHA(LONG           , xDest, D2),
        AROS_LHA(LONG           , yDest, D3),
        AROS_LHA(LONG           , xSize, D4),
        AROS_LHA(LONG           , ySize, D5),
        AROS_LHA(ULONG          , opacity, D6),

/*  LOCATION */
        struct GfxBase *, GfxBase, 205, Graphics)

/*  FUNCTION
        Blends a rectangle of srcBitMap over destBitMap, Porter-Duff
        "source over": the source's per pixel alpha channel, scaled by the
        constant opacity, is the coverage of every pixel. A destination
        with an alpha channel of its own has it taken into account and
        updated; a destination without one is treated as opaque.

        A source bitmap without an alpha channel reads as fully opaque, so
        with an opacity of 255 the call is a plain copy (like BltBitMap()
        with minterm 0xC0), and with a lower opacity a constant blend of
        the source over the destination.

        The rectangle is clipped to both bitmaps. Nothing is drawn for an
        opacity of 0.

    INPUTS
        srcBitMap    - Blend from this bitmap.
        xSrc, ySrc   - Upper left corner of the area to blend.
        destBitMap   - Blend over this bitmap. Must not be srcBitMap, and
                       the two rectangles must not overlap when both are
                       in the same bitmap memory.
        xDest, yDest - Upper left corner where to blend to.
        xSize, ySize - Size of the area.
        opacity      - Constant alpha applied on top of the source's alpha
                       channel: 0 (invisible) to 255 (the source's own
                       alpha only). Larger values are treated as 255.

    RESULT
        None.

    NOTES
        This function is an AROS extension; it is not present in AmigaOS
        or MorphOS graphics.library. Check GfxBase->LibNode.lib_Version
        >= 45 and, when running on other systems is a concern, test for
        the function with its LVO before calling it.

        Both bitmaps have to be bitmaps the display driver knows (allocated
        with AllocBitMap() with a friend bitmap, a display mode or
        BMF_SPECIALFMT, i.e. not hand made planar bitmaps). For planar
        bitmaps an opacity of 255 falls back to BltBitMap() with minterm
        0xC0 (a copy of all planes) and any other opacity does nothing, as
        planar bitmaps have no alpha channel and no blending.

        A displayable destination (a screen bitmap, or the surface of a
        composited layer) is updated on the display like after any other
        blit; the function is the application level access to the display
        driver's blend, the same the display uses to compose translucent
        layers.

        Unlike BltBitMap() the coordinates and sizes are LONGs.

    EXAMPLE
        /- blend a 32 bit ARGB image, prepared with WritePixelArray()
           (RECTFMT_ARGB), half transparent over a screen bitmap -/
        BltBitMapAlpha(image, 0, 0, screen->RastPort.BitMap, 100, 100,
                       width, height, 128);

    BUGS

    SEE ALSO
        BltBitMap(), BltMaskBitMapRastPort(), cybergraphics.library/WritePixelArrayAlpha()

    INTERNALS
        Calls moHidd_Gfx_CopyBoxAlpha of the destination bitmap's display
        driver; the base class fetches the source as ARGB32 rows and puts
        them with moHidd_BitMap_PutAlphaImage, so any driver that
        accelerates one of those accelerates this too.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG wSrc, hSrc, wDest, hDest;
    struct gfxdisplay_data *driver;
    OOP_Object *gc, *src_obj, *dst_obj;

    D(bug("BltBitMapAlpha(%p, %ld, %ld, %p, %ld, %ld, %ld, %ld, %lu)\n",
          srcBitMap, xSrc, ySrc, destBitMap, xDest, yDest, xSize, ySize, opacity));

    if (!srcBitMap || !destBitMap)
        return;

    if (opacity > 255)
        opacity = 255;

    if (0 == opacity || xSize <= 0 || ySize <= 0)
        return;

    if (!IS_HIDD_BM(srcBitMap) || !IS_HIDD_BM(destBitMap))
    {
        /* Planar bitmaps have no alpha channel: opaque is a copy, anything else nothing */
        if (255 == opacity)
            BltBitMap(srcBitMap, xSrc, ySrc, destBitMap, xDest, yDest, xSize, ySize, 0xC0, 0xFF, NULL);
        return;
    }

    wSrc  = GetBitMapAttr(srcBitMap, BMA_WIDTH);
    hSrc  = GetBitMapAttr(srcBitMap, BMA_HEIGHT);
    wDest = GetBitMapAttr(destBitMap, BMA_WIDTH);
    hDest = GetBitMapAttr(destBitMap, BMA_HEIGHT);

    /* Clip to both bitmaps; a corner moved in on one side moves on the other too */
    if (xSrc < 0)
    {
        xDest -= xSrc;
        xSize += xSrc;
        xSrc = 0;
    }
    if (ySrc < 0)
    {
        yDest -= ySrc;
        ySize += ySrc;
        ySrc = 0;
    }
    if (xDest < 0)
    {
        xSrc -= xDest;
        xSize += xDest;
        xDest = 0;
    }
    if (yDest < 0)
    {
        ySrc -= yDest;
        ySize += yDest;
        yDest = 0;
    }
    if (xSrc + xSize > wSrc)
        xSize = wSrc - xSrc;
    if (ySrc + ySize > hSrc)
        ySize = hSrc - ySrc;
    if (xDest + xSize > wDest)
        xSize = wDest - xDest;
    if (yDest + ySize > hDest)
        ySize = hDest - yDest;

    if (xSize <= 0 || ySize <= 0)
        return;

    /*
     * The destination's driver blends. The base class implementation reads
     * the source through its bitmap object, so the source may belong to
     * another driver.
     */
    driver  = GET_BM_DRIVERDATA(destBitMap);
    src_obj = HIDD_BM_OBJ(srcBitMap);
    dst_obj = HIDD_BM_OBJ(destBitMap);

    gc = obtain_cache_object(CDD(GfxBase)->gc_cache, GfxBase);
    if (NULL != gc)
    {
        HIDD_Gfx_CopyBoxAlpha(driver->display_gfxhidd, src_obj, xSrc, ySrc, dst_obj, xDest, yDest,
                              xSize, ySize, opacity, gc);
        update_bitmap(destBitMap, dst_obj, xDest, yDest, xSize, ySize, GfxBase);
        release_cache_object(CDD(GfxBase)->gc_cache, gc, GfxBase);
    }

    AROS_LIBFUNC_EXIT
} /* BltBitMapAlpha */
