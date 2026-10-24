/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Alpha of one pixel of a composited layer's surface.
          AROS private, for layers.library.
*/

#include <graphics/gfx.h>
#include <hidd/gfx.h>
#include <proto/oop.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

#ifndef AROS_NO_COMPOSITED_LAYERS
#define AROS_NO_COMPOSITED_LAYERS 0
#endif

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH3(ULONG, LayerSurfaceAlpha,

/*  SYNOPSIS */
        AROS_LHA(struct BitMap *, surface, A0),
        AROS_LHA(LONG           , x, D0),
        AROS_LHA(LONG           , y, D1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 204, Graphics)

/*  FUNCTION
        Returns the alpha (coverage) of one pixel of a bitmap with an
        alpha channel, scaled to 0 (transparent) .. 255 (opaque).
        layers.library uses it for the hit test of composited layers that
        have a per pixel alpha surface (LA_AlphaSurface): a fully
        transparent pixel does not hit the layer.

    INPUTS
        surface - the bitmap, normally the surface of a composited layer
        x, y    - the pixel, bitmap relative

    RESULT
        The pixel's alpha, 0..255. 255 when the bitmap has no alpha
        channel (nothing can be seen through it), is not a bitmap the
        display driver knows, or the pixel is outside the bitmap.

    NOTES
        AROS private, for layers.library. Can be moved at any moment.

    SEE ALSO
        CompositeLayersSupported(), LayerSurfacesChanged(),
        layers.library/IsLayerHitAt()

    INTERNALS
        Reads the pixel with moHidd_BitMap_GetPixel and takes the alpha
        component apart with the pixel format's AlphaMask/AlphaShift, the
        same way ALPHA_COMP() does: the component left aligned in the 32
        bit pixel word, i.e. 16 bits wide, of which the top 8 are returned.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
#if AROS_NO_COMPOSITED_LAYERS
    /* ROM-size build: no layer surfaces exist, everything is opaque */
    return 255;
#else

    OOP_Object *bm, *pf = NULL;
    IPTR mask = 0, shift = 0;
    HIDDT_Pixel pix, comp;

    if (!surface || !IS_HIDD_BM(surface))
        return 255;

    if (x < 0 || y < 0 || x >= GetBitMapAttr(surface, BMA_WIDTH) || y >= GetBitMapAttr(surface, BMA_HEIGHT))
        return 255;

    bm = HIDD_BM_OBJ(surface);
    OOP_GetAttr(bm, aHidd_BitMap_PixFmt, (IPTR *)&pf);
    if (!pf)
        return 255;

    OOP_GetAttr(pf, aHidd_PixFmt_AlphaMask, &mask);
    OOP_GetAttr(pf, aHidd_PixFmt_AlphaShift, &shift);
    if (0 == mask)
        return 255;

    pix  = HIDD_BM_GetPixel(bm, x, y);
    comp = ((pix & (HIDDT_Pixel)mask) << shift) >> MAP_SHIFT;     /* a HIDDT_ColComp */

    return (ULONG)(comp >> (sizeof(HIDDT_ColComp) * 8 - 8)) & 0xFF;
#endif

    AROS_LIBFUNC_EXIT
} /* LayerSurfaceAlpha */
