/*
    Copyright © 2003, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <intuition/imageclass.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>

#include <stdio.h>
#include <proto/dos.h>

#include "mui.h"
#include "imspec_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

STATIC VOID TrueDitherV
(
    struct RastPort *rp,
    WORD x1, WORD y1, WORD x2, WORD y2,
    WORD oy1, WORD oy2,
    ULONG *start_rgb, ULONG *end_rgb
)
{
    LONG max_delta_y = oy2 - oy1;
    LONG width       = x2  - x1 + 1;

    LONG delta_r = end_rgb[0] - start_rgb[0];
    LONG delta_g = end_rgb[1] - start_rgb[1];
    LONG delta_b = end_rgb[2] - start_rgb[2];

    LONG y;

    for(y = y1; y <= y2; y++)
    {
        const LONG delta_y = y - oy1;

        LONG red   = start_rgb[0];
	LONG green = start_rgb[1];
	LONG blue  = start_rgb[2];

        red   += (delta_y * delta_r)/max_delta_y;
        green += (delta_y * delta_g)/max_delta_y;
        blue  += (delta_y * delta_b)/max_delta_y;

        FillPixelArray(rp, x1, y, width, 1, (red << 16) + (green << 8) + blue);
    }
}

STATIC VOID TrueDitherH
(
    struct RastPort *rp,
    WORD x1, WORD y1, WORD x2, WORD y2,
    WORD ox1, WORD ox2,
    ULONG *start_rgb, ULONG *end_rgb
)
{
    LONG max_delta_x = ox2 - ox1;
    LONG height      = y2  - y1 + 1;

    LONG delta_r = end_rgb[0] - start_rgb[0];
    LONG delta_g = end_rgb[1] - start_rgb[1];
    LONG delta_b = end_rgb[2] - start_rgb[2];

    LONG x;

    for(x = x1; x <= x2; x++)
    {
        const LONG delta_x = x - ox1;

        LONG red   = start_rgb[0];
	LONG green = start_rgb[1];
	LONG blue  = start_rgb[2];

        red   += (delta_x * delta_r)/max_delta_x;
        green += (delta_x * delta_g)/max_delta_x;
        blue  += (delta_x * delta_b)/max_delta_x;

        FillPixelArray(rp, x, y1, 1, height, (red << 16) + (green << 8) + blue);
#if 0
        kprintf("r = %08lx\n"
                "g = %08lx\n"
                "b = %08lx\n"
                "ox1 = %d\n"
                "ox2 = %d\n"
                "x1 = %d\n"
                "x2 = %d\n"
                "x   = %ld\n"
                "max_delta_x = %ld\n"
                "delta_x     = %ld\n\n", red, green, blue, ox1, ox2, x1, x2, x, max_delta_x, delta_x);

        Delay(100);
#endif
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
                spec->u.gradient.y1, spec->u.gradient.y2,
                spec->u.gradient.start_rgb, spec->u.gradient.end_rgb
            );
            break;

        case 'h':
            TrueDitherH
            (
                mri->mri_RastPort,
                x1, y1, x2, y2,
                spec->u.gradient.x1, spec->u.gradient.x2,
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
