/*
    Copyright © 2003, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <intuition/imageclass.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>

#include <stdio.h>

#include "mui.h"
#include "imspec_intern.h"
#include "support.h"

#include <aros/debug.h>

extern struct Library *MUIMasterBase;

STATIC VOID TrueDitherV
(
    struct RastPort *rp,
    WORD x1, WORD y1, WORD x2, WORD y2,
    WORD xoff, WORD yoff,
    ULONG *start_rgb, ULONG *end_rgb
)
{
    LONG width;
    LONG height_minus_one;

    ULONG red   = start_rgb[0];
    ULONG green = start_rgb[1];
    ULONG blue  = start_rgb[2];

    LONG delta_r = end_rgb[0] - start_rgb[0];
    LONG delta_g = end_rgb[1] - start_rgb[1];
    LONG delta_b = end_rgb[2] - start_rgb[2];

    LONG y;

    x1 += yoff;
    if (x1 > x2) x1 = x2;

    width           = x2 - x1 + 1;
    height_minus_one= y2 - y1;

    if (yoff == 0)
    {
        FillPixelArray(rp, x1, y1, width, 1, (red << 16) + (green << 8) + blue);
        yoff = 1;
    }

    for(y = yoff; y <= height_minus_one; y++)
    {
        red   += (y * delta_r)/height_minus_one;
        green += (y * delta_g)/height_minus_one;
        blue  += (y * delta_b)/height_minus_one;

        FillPixelArray(rp, x1 , y1 + y, width, 1, (red << 16) + (green << 8) + blue);

        red   = start_rgb[0];
	green = start_rgb[1];
	blue  = start_rgb[2];
    }
}

STATIC VOID TrueDitherH
(
    struct RastPort *rp,
    WORD x1, WORD y1, WORD x2, WORD y2,
    WORD xoff, WORD yoff,
    ULONG *start_rgb, ULONG *end_rgb
)
{
    LONG width_minus_one;
    LONG height;

    ULONG red   = start_rgb[0];
    ULONG green = start_rgb[1];
    ULONG blue  = start_rgb[2];

    LONG delta_r = end_rgb[0] - start_rgb[0];
    LONG delta_g = end_rgb[1] - start_rgb[1];
    LONG delta_b = end_rgb[2] - start_rgb[2];

    LONG x;

    y1 += yoff;
    if (y1 > y2) y1 = y2;

    width_minus_one = x2 - x1;
    height          = y2 - y1 + 1;

    if (xoff == 0)
    {
        FillPixelArray(rp, x1, y1, 1, height, (red << 16) + (green << 8) + blue);

        xoff = 1;
    }

    for(x = xoff; x <= width_minus_one; x++)
    {
        red   += (x * delta_r)/width_minus_one;
        green += (x * delta_g)/width_minus_one;
        blue  += (x * delta_b)/width_minus_one;

        FillPixelArray(rp, x1 + x, y1, 1, height, (red << 16) + (green << 8) + blue);

        red   = start_rgb[0];
	green = start_rgb[1];
	blue  = start_rgb[2];
    }
}

/***************************************************************************************************/

VOID zune_gradient_draw
(
    struct MUI_ImageSpec_intern *spec,
    struct MUI_RenderInfo *mri,
    WORD x1, WORD y1, WORD x2, WORD y2,
    WORD xoff, WORD yoff
)
{
    if (!(CyberGfxBase && (GetBitMapAttr(mri->mri_RastPort->BitMap, BMA_DEPTH) >= 15)))
        return;

    switch(spec->u.gradient.orientation)
    {
        case 'v':
            TrueDitherV
            (
                mri->mri_RastPort,
                x1, y1, x2, y2,
                xoff, yoff,
                spec->u.gradient.start_rgb, spec->u.gradient.end_rgb
            );
            break;

        case 'h':
            TrueDitherH
            (
                mri->mri_RastPort,
                x1, y1, x2, y2,
                xoff, yoff,
                spec->u.gradient.start_rgb, spec->u.gradient.end_rgb
            );
            break;

    } /* switch(orientation) */
}

BOOL zune_gradient_string_to_intern(CONST_STRPTR str,
                                     struct MUI_ImageSpec_intern *spec)
{
    UBYTE orientation;
    ULONG start_r, start_g, start_b;
    ULONG end_r, end_g, end_b;

    if
    (
        !str
     || !spec
     || (sscanf(str, "%c,%8lx,%8lx,%8lx-%8lx,%8lx,%8lx",
                     &orientation, &start_r, &start_g, &start_b,
                     &end_r, &end_g, &end_b) < 7)
    )
    {
        return FALSE;
    }

    switch (orientation)
    {
        case 'H':
        case 'h':
            spec->u.gradient.orientation = 'h';
            break;

        case 'V':
        case 'v':
            spec->u.gradient.orientation = 'v';
            break;

        default:
            return FALSE;
    }

    spec->u.gradient.start_rgb[0] = start_r>>24;
    spec->u.gradient.start_rgb[1] = start_g>>24;
    spec->u.gradient.start_rgb[2] = start_b>>24;

    spec->u.gradient.end_rgb[0] = end_r>>24;
    spec->u.gradient.end_rgb[1] = end_g>>24;
    spec->u.gradient.end_rgb[2] = end_b>>24;

    return TRUE;


}

VOID zune_gradient_intern_to_string(struct MUI_ImageSpec_intern *spec,
                                    STRPTR buf)
{
    sprintf(buf, "7:%c,%08lx,%08lx,%08lx-%08lx,%08lx,%08lx",
                 spec->u.gradient.orientation,
                 spec->u.gradient.start_rgb[0]*0x01010101,
                 spec->u.gradient.start_rgb[1]*0x01010101,
                 spec->u.gradient.start_rgb[2]*0x01010101,
                 spec->u.gradient.end_rgb[0]*0x01010101,
                 spec->u.gradient.end_rgb[1]*0x01010101,
                 spec->u.gradient.end_rgb[2]*0x01010101);
}
