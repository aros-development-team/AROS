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

/*
   sizeof(LONG)*8 gives the size in bits of the LONG type,
   then 8+1 bits (a color component size in bits, plus one bit
   to account for the sign of the delta) are subtracted from it,
   the result is the number of bits which can be used for the fractional
   part to do fixed point math with the maximum precision.

   Using all the remaining bits for the fractional part, IN THIS SPECIAL CASE,
   does not incurr in overflow problems for the integer part, for the way
   we use fractional numbers in this algorithm. Basically, we first scale up
   a number A, and then we divide A by the number B. Successively,
   the only operation we do on the number A is adding it to the number C,
   initially equal to 0, _at maximum_ B times, which means that overflow
   never happens.

*/

#define SHIFT (sizeof(ULONG)*8 - 9)

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

    LONG step_r = (delta_r << SHIFT)/max_delta_y, cur_r = (y1 - oy1)*step_r;
    LONG step_g = (delta_g << SHIFT)/max_delta_y, cur_g = (y1 - oy1)*step_g;
    LONG step_b = (delta_b << SHIFT)/max_delta_y, cur_b = (y1 - oy1)*step_b;

    for(y = y1; y <= y2; y++)
    {
        const LONG red   = start_rgb[0] + (cur_r >> SHIFT);
	const LONG green = start_rgb[1] + (cur_g >> SHIFT);
	const LONG blue  = start_rgb[2] + (cur_b >> SHIFT);

        FillPixelArray(rp, x1, y, width, 1, (red << 16) + (green << 8) + blue);

        cur_r += step_r;
        cur_g += step_g;
        cur_b += step_b;
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

    LONG step_r = (delta_r << SHIFT)/max_delta_x, cur_r = (x1 - ox1)*step_r;
    LONG step_g = (delta_g << SHIFT)/max_delta_x, cur_g = (x1 - ox1)*step_g;
    LONG step_b = (delta_b << SHIFT)/max_delta_x, cur_b = (x1 - ox1)*step_b;

    for(x = x1; x <= x2; x++)
    {
        const LONG red   = start_rgb[0] + (cur_r >> SHIFT);
	const LONG green = start_rgb[1] + (cur_g >> SHIFT);
	const LONG blue  = start_rgb[2] + (cur_b >> SHIFT);

        FillPixelArray(rp, x, y1, 1, height, (red << 16) + (green << 8) + blue);

        cur_r += step_r;
        cur_g += step_g;
        cur_b += step_b;
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
