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

   UPDATE: I've come up with a solution which makes this algorithm
           twice as fast as before, by taking advantage of the fact that
           a + b/c = (ac + b)/c, which means that now the number C is set
           to an initial value different than 0. There's no need to diminish
           the number of fractional bits as, for the way the algorithm
           works, there's still no overflow.

*/

#define SHIFT (sizeof(ULONG)*8 - (8 + 1))

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

    LONG step_r = (delta_r << SHIFT)/max_delta_y;
    LONG step_g = (delta_g << SHIFT)/max_delta_y;
    LONG step_b = (delta_b << SHIFT)/max_delta_y;

    LONG y, offset_y = y1 - oy1;

    LONG red   = ((1 << SHIFT) >> 1) + (start_rgb[0] << SHIFT) + offset_y*step_r;
    LONG green = ((1 << SHIFT) >> 1) + (start_rgb[1] << SHIFT) + offset_y*step_g;
    LONG blue  = ((1 << SHIFT) >> 1) + (start_rgb[2] << SHIFT) + offset_y*step_b;

    for(y = y1; y <= y2; y++)
    {
        FillPixelArray(rp, x1, y, width, 1,
                       ((red >> SHIFT) << 16) + ((green >> SHIFT) << 8) + (blue >> SHIFT));

        red   += step_r;
        green += step_g;
        blue  += step_b;
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

    LONG step_r = (delta_r << SHIFT)/max_delta_x;
    LONG step_g = (delta_g << SHIFT)/max_delta_x;
    LONG step_b = (delta_b << SHIFT)/max_delta_x;

    LONG x, offset_x = x1 - ox1;

    /* 1 << (SHIFT - 1) is 0.5 in fixed point math. We add it to the variable
       so that, at the moment in which the variable is converted to integer,
       rounding is done properly. That is, a+x, with 0 < x < 0.5, is rounded
       down to a, and a+x, with 0.5 <= x < 1, is rounded up to a+1. */

    LONG red   = ((1 << SHIFT) >> 1) + (start_rgb[0] << SHIFT) + offset_x*step_r;
    LONG green = ((1 << SHIFT) >> 1) + (start_rgb[1] << SHIFT) + offset_x*step_g;
    LONG blue  = ((1 << SHIFT) >> 1) + (start_rgb[2] << SHIFT) + offset_x*step_b;

    for(x = x1; x <= x2; x++)
    {
        FillPixelArray(rp, x, y1, 1, height,
                       ((red >> SHIFT) << 16) + ((green >> SHIFT) << 8) + (blue >> SHIFT));

        red   += step_r;
        green += step_g;
        blue  += step_b;
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
    ULONG *start_rgb = spec->u.gradient.start_rgb;
    ULONG *end_rgb   = spec->u.gradient.end_rgb;

    if (!(CyberGfxBase && (GetBitMapAttr(mri->mri_RastPort->BitMap, BMA_DEPTH) >= 15)))
        return;

    switch(spec->u.gradient.orientation)
    {
        case 'v':
        {
            LONG oy1 = _mtop(spec->u.gradient.obj), oy2 = _mbottom(spec->u.gradient.obj);
            LONG delta_oy = oy2 - oy1;
            LONG hh = (delta_oy + 1)*2;
            LONG mid_y;

            yoff %= hh;

            if (yoff < 0)
                yoff += hh;

            oy1 -= yoff; oy2 -= yoff;

            if (y2 > oy2)
            {
                mid_y = y1 + delta_oy - yoff;

                if (yoff > delta_oy)
                {
                    ULONG *tmp = start_rgb;
                    start_rgb  = end_rgb;
                    end_rgb    = tmp;

                    mid_y += delta_oy;
                    oy1   += delta_oy;
                    oy2   += delta_oy;
                }
            }
            else
            {
                mid_y = y2;
            }

            TrueDitherV
            (
                mri->mri_RastPort,
                x1, y1, x2, mid_y,
                oy1, oy2,
                start_rgb, end_rgb
            );

            if (mid_y < y2)
            {
                TrueDitherV
                (
                    mri->mri_RastPort,
                    x1, mid_y+1, x2, y2,
                    oy1+delta_oy, oy2+delta_oy,
                    end_rgb, start_rgb
                );
            }

            break;
        }
        case 'h':
        {
            LONG ox1 = _mleft(spec->u.gradient.obj), ox2 = _mright(spec->u.gradient.obj);
            LONG delta_ox = ox2 - ox1;
            LONG ww = (delta_ox + 1)*2;
            LONG mid_x;


            xoff %= ww;
            if (xoff < 0)
                xoff += ww;

            ox1 -= xoff; ox2 -= xoff;

            if (x2 > ox2)
            {
                mid_x = x1 + delta_ox - xoff;

                if (xoff > delta_ox)
                {
                    ULONG *tmp = start_rgb;
                    start_rgb  = end_rgb;
                    end_rgb    = tmp;

                    mid_x += delta_ox;
                    ox1   += delta_ox;
                    ox2   += delta_ox;
                }
            }
            else
            {
                mid_x = x2;
            }

            TrueDitherH
            (
                mri->mri_RastPort,
                x1, y1, mid_x, y2,
                ox1, ox2,
                start_rgb, end_rgb
            );

            if (mid_x < x2)
            {
                TrueDitherH
                (
                    mri->mri_RastPort,
                    mid_x+1, y1, x2, y2,
                    ox1+delta_ox, ox2+delta_ox,
                    end_rgb, start_rgb
                );
            }

            break;
        }
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
