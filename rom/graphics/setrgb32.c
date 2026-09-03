/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Set one color register for this Viewport
*/

#include <aros/debug.h>
#include <proto/oop.h>
#include <proto/graphics.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

void internal_SetRGB32Colors(struct ViewPort *vp, HIDDT_Color *colors,
                             ULONG first, ULONG count,
                             struct GfxBase *GfxBase)
{
    struct ViewPortExtra *vpe = NULL;
    OOP_Object *bm;
    OOP_Object *pf;
    HIDDT_ColorModel colmod;
    ULONG i;

    if (vp->ColorMap)
        vpe = vp->ColorMap->cm_vpe;

    if (IS_HIDD_BM(vp->RasInfo->BitMap))
        bm = HIDD_BM_OBJ(vp->RasInfo->BitMap);
    else
    {
        if (!vpe)
            vpe = (struct ViewPortExtra *)GfxLookUp(vp);

        if ((!vpe) || (!VPE_DATA(vpe)))
            return;

        bm = VPE_DATA(vpe)->Bitmap;
    }

    OOP_GetAttr(bm, aHidd_BitMap_PixFmt, (IPTR *)&pf);
    OOP_GetAttr(pf, aHidd_PixFmt_ColorModel, &colmod);

    if (vHidd_ColorModel_Palette == colmod ||
        vHidd_ColorModel_TrueColor == colmod)
    {
        HIDD_BM_SetColors(bm, colors, first, count);

        if (IS_HIDD_BM(vp->RasInfo->BitMap))
        {
            for (i = 0; i < count; i++)
                HIDD_BM_PIXTAB(vp->RasInfo->BitMap)[first + i] =
                    colors[i].pixval;
        }
    }
}

/*****************************************************************************

    NAME */
#include <graphics/view.h>
#include <proto/graphics.h>

        AROS_LH5(void, SetRGB32,

/*  SYNOPSIS */
        AROS_LHA(struct ViewPort *, vp, A0),
        AROS_LHA(ULONG            , n, D0),
        AROS_LHA(ULONG            , r, D1),
        AROS_LHA(ULONG            , g, D2),
        AROS_LHA(ULONG            , b, D3),

/*  LOCATION */
        struct GfxBase *, GfxBase, 142, Graphics)

/*  FUNCTION
        Changes a single color of a viewport.

    INPUTS
        vp - Modify this viewport
        n - Change this color. If the color is outside the range of
                valid colors, it will be ignored.
        r, g, b - The new values for the red, green and blue. The
                valid range is from 0x000000 (no intensity) to
                0xFFFFFFFF (full intensity).

    RESULT
        If there is a ColorMap for this viewport, then the value will
        be stored in the ColorMap.
        The selected color register is changed to match your specs.
        If the color value is unused then nothing will happen.

    NOTES
        Lower order bits of the palette specification will be discarded,
        depending on the color palette resolution of the target graphics
        device. Use 0xffffffff for the full value, 0x7fffffff for 50%,
        etc. You can find out the palette range for your screen by
        querying the graphics data base.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    HIDDT_Color hidd_col;

    if(vp->ColorMap)
        SetRGB32CM(vp->ColorMap, n, r, g, b);

    /* HIDDT_Color entries are UWORD */
    hidd_col.red   = r >> 16;
    hidd_col.green = g >> 16;
    hidd_col.blue  = b >> 16;
    hidd_col.alpha = 0;

    internal_SetRGB32Colors(vp, &hidd_col, n, 1, GfxBase);

    D(bug("SetRGB32: bm %p, col %d (%x %x %x %x) mapped to %x\n"
          , vp->RasInfo->BitMap
          , n
          , hidd_col.red, hidd_col.green, hidd_col.blue, hidd_col.alpha
          , hidd_col.pixval));

    AROS_LIBFUNC_EXIT

} /* SetRGB32 */
