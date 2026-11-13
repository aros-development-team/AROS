/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/debug.h>
#include <clib/macros.h>
#include <hidd/gfx.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include "cybergraphics_intern.h"
#include "gfxfuncsupport.h"

extern ULONG extcol_render(struct extcol_render_data *ecrd,
                           LONG x_src, LONG y_src,
                           OOP_Object *bm_obj, OOP_Object *dst_gc,
                           struct Rectangle *rect, struct GfxBase *GfxBase);

/*****************************************************************************

    NAME */
#include <proto/cybergraphics.h>

        AROS_LH7(ULONG, ExtractColor,

/*  SYNOPSIS */
        AROS_LHA(struct RastPort *, RastPort, A0),
        AROS_LHA(struct BitMap   *, SingleMap, A1),
        AROS_LHA(ULONG            , Colour, D0),
        AROS_LHA(ULONG            , sX, D1),
        AROS_LHA(ULONG            , sY, D2),
        AROS_LHA(ULONG            , Width, D3),
        AROS_LHA(ULONG            , Height, D4),

/*  LOCATION */
        struct Library *, CyberGfxBase, 31, Cybergraphics)

/*  FUNCTION
        Create a single-plane bitmap that describes the coordinates where a
        particular colour is present in a portion of a RastPort (i.e. a mask).
        A one is stored in the bitmap where the requested colour is present,
        and a zero where it is absent.

        For true-colour RastPorts, the colour is specified in 32-bit ARGB
        format: 1 byte per component, in the order alpha, red, green, blue
        (the alpha byte is ignored for RastPorts without an alpha channel).
        For paletted RastPorts, a pen number is given instead.

    INPUTS
        RastPort - the RastPort to analyse.
        SingleMap - a planar bitmap to fill (its pixel dimensions must be at
            least as big as the rectangle being analysed).
        Colour - the colour to extract.
        sX, sY - top-lefthand corner of portion of RastPort to analyse
            (in pixels).
        Width, Height - size of the area to analyse (in pixels).

    RESULT
        result - Boolean success indicator.

    NOTES
        It is safe for the bitmap being filled to have more than one bitplane.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Rectangle rr;
    LONG pixread = 0;
    struct extcol_render_data ecrd;
    OOP_Object *pf;
    IPTR colmod;

    if (!IS_HIDD_BM(RastPort->BitMap))
    {
        D(bug("!!! CALLING ExtractColor() ON NO-HIDD BITMAP !!!\n"));
        return FALSE;
    }

    rr.MinX = sX;
    rr.MinY = sY;
    rr.MaxX = sX + Width  - 1;
    rr.MaxY = sY + Height - 1;

    OOP_GetAttr(HIDD_BM_OBJ(RastPort->BitMap), aHidd_BitMap_PixFmt, (IPTR *)&pf);
    OOP_GetAttr(pf, aHidd_PixFmt_ColorModel, (IPTR *)&colmod);

    if (vHidd_ColorModel_Palette == colmod)
    {
        ecrd.pixel = Colour;
        ecrd.mask  = ~(HIDDT_Pixel)0;
    }
    else
    {
        IPTR r = 0, g = 0, b = 0, a = 0;
        HIDDT_Color col;

        col.alpha = (Colour >> 16) & 0x0000FF00;
        col.red   = (Colour >> 8 ) & 0x0000FF00;
        col.green = Colour & 0x0000FF00;
        col.blue  = (Colour << 8) & 0x0000FF00;

        ecrd.pixel = HIDD_BM_MapColor(HIDD_BM_OBJ(RastPort->BitMap), &col);

        /*
         * Only the bits of the colour components count: the pad byte of a
         * 32 bit format without alpha (0RGB32, BGR032, ...) is undefined.
         */
        OOP_GetAttr(pf, aHidd_PixFmt_RedMask,   &r);
        OOP_GetAttr(pf, aHidd_PixFmt_GreenMask, &g);
        OOP_GetAttr(pf, aHidd_PixFmt_BlueMask,  &b);
        OOP_GetAttr(pf, aHidd_PixFmt_AlphaMask, &a);
        ecrd.mask = (HIDDT_Pixel)(r | g | b | a);
        ecrd.pixel &= ecrd.mask;
    }

    ecrd.destbm       = SingleMap;
    ecrd.CyberGfxBase = (struct IntCGFXBase *)CyberGfxBase;

    pixread = DoRenderFunc(RastPort, NULL, &rr, extcol_render, &ecrd, FALSE);

    /*
     * In our render callback we return one if everything went ok.
     * DoRenderFunc() sums up all return values from callback. So, if all failed,
     * this will be zero. Otherwise it will indicate how many times the callback
     * was called.
     */
    return (pixread > 0);

    AROS_LIBFUNC_EXIT
} /* ExtractColor */
