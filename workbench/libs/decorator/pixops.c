/*
    Copyright (C) 2026, The AROS Development Team.

    Desc: decorator.library - row-level pixel kernels, generic C implementations

    These functions are the hot inner loops of the rendering support code.
    Architectures may provide optimized (e.g. SIMD) replacements by building
    an arch specific version of this file - see arch/x86_64-all/decorator/.
    Replacements must produce identical output to these implementations.
*/

#include <exec/types.h>

#include <libraries/decorator.h>
#include "decorator_intern.h"

void pixop_splat_row(ULONG *d, LONG n, ULONG px)
{
    LONG i;

    for (i = 0; i < n; i++)
        d[i] = px;
}

void pixop_extract_alpha_row(UBYTE *d, const ULONG *s, LONG n)
{
    LONG i;

    for (i = 0; i < n; i++)
        d[i] = GET_ARGB_A(s[i]);
}

void pixop_tint_row(ULONG *d, LONG n, ULONG argb, UWORD ratio)
{
    UBYTE   rs = (argb >> 16) & 0xff;
    UBYTE   gs = (argb >> 8) & 0xff;
    UBYTE   bs = argb & 0xff;
    UWORD   cr = (rs * ratio) >> 8;
    UWORD   cg = (gs * ratio) >> 8;
    UWORD   cb = (bs * ratio) >> 8;
    UWORD   inv = 255 - ratio;
    ULONG   rgb;
    UWORD   r, g, b;
    LONG    i;

    for (i = 0; i < n; i++)
    {
        rgb = d[i];
        r = cr + ((GET_ARGB_R(rgb) * inv) >> 8);
        g = cg + ((GET_ARGB_G(rgb) * inv) >> 8);
        b = cb + ((GET_ARGB_B(rgb) * inv) >> 8);
        d[i] = SET_ARGB(255, r, g, b);
    }
}

void pixop_mix_row(ULONG *d, const ULONG *s, LONG n, UWORD ratio, BOOL tiled)
{
    UWORD   inv = 255 - ratio;
    ULONG   rgba, rgb;
    UBYTE   as, rs, gs, bs, rd, gd, bd;
    UWORD   r, g, b;
    LONG    i;

    for (i = 0; i < n; i++)
    {
        rgba = s[i];
        as = GET_ARGB_A(rgba);
        rs = GET_ARGB_R(rgba);
        gs = GET_ARGB_G(rgba);
        bs = GET_ARGB_B(rgba);
        rgb = d[i];
        rd = GET_ARGB_R(rgb);
        gd = GET_ARGB_G(rgb);
        bd = GET_ARGB_B(rgb);

        r = ((rs * ratio) >> 8) + ((rd * inv) >> 8);
        g = ((gs * ratio) >> 8) + ((gd * inv) >> 8);
        b = ((bs * ratio) >> 8) + ((bd * inv) >> 8);

        if (tiled)
        {
            r = ((r * as) >> 8) + ((rd * (255 - as)) >> 8);
            g = ((g * as) >> 8) + ((gd * (255 - as)) >> 8);
            b = ((b * as) >> 8) + ((bd * (255 - as)) >> 8);
        }

        d[i] = SET_ARGB(as, r, g, b);
    }
}

void pixop_shade_row(ULONG *d, const ULONG *s, LONG n, UWORD fact)
{
    ULONG   rgb;
    ULONG   c0, c1, c2, c3;
    LONG    i;

    for (i = 0; i < n; i++)
    {
        rgb = s[i];
        c0 = (GET_ARGB_A(rgb) * fact) >> 8;
        c1 = (GET_ARGB_R(rgb) * fact) >> 8;
        c2 = (GET_ARGB_G(rgb) * fact) >> 8;
        c3 = (GET_ARGB_B(rgb) * fact) >> 8;
        if (c0 > 255) c0 = 255;
        if (c1 > 255) c1 = 255;
        if (c2 > 255) c2 = 255;
        if (c3 > 255) c3 = 255;
        d[i] = SET_ARGB(c0, c1, c2, c3);
    }
}

void pixop_blur14mix_row(ULONG *d, const ULONG *rm2, const ULONG *rm1, const ULONG *r0,
                         const ULONG *rp1, const ULONG *rp2, const ULONG *tex, LONG w)
{
    LONG    x, l1, l2, r1, r2;
    ULONG   rgb, argb;
    UWORD   red, green, blue, as, rs, gs, bs;

    for (x = 0; x < w; x++)
    {
        l1 = 1;
        l2 = 2;
        r1 = 1;
        r2 = 2;

        if (x == 0) l1 = l2 = 0;
        else if (x == 1) l2 = l1;

        if (x == (w - 1)) r1 = r2 = 0;
        else if (x == (w - 2)) r2 = r1;

        rgb = r0[x];
        red = GET_ARGB_R(rgb);
        green = GET_ARGB_G(rgb);
        blue = GET_ARGB_B(rgb);

        rgb = rm2[x];
        red += GET_ARGB_R(rgb);
        green += GET_ARGB_G(rgb);
        blue += GET_ARGB_B(rgb);

        rgb = rm1[x - l1];
        red += GET_ARGB_R(rgb);
        green += GET_ARGB_G(rgb);
        blue += GET_ARGB_B(rgb);

        rgb = rm1[x];
        red += 2 * GET_ARGB_R(rgb);
        green += 2 * GET_ARGB_G(rgb);
        blue += 2 * GET_ARGB_B(rgb);

        rgb = rm1[x + r1];
        red += GET_ARGB_R(rgb);
        green += GET_ARGB_G(rgb);
        blue += GET_ARGB_B(rgb);

        rgb = r0[x - l2];
        red += GET_ARGB_R(rgb);
        green += GET_ARGB_G(rgb);
        blue += GET_ARGB_B(rgb);

        rgb = r0[x - l1];
        red += GET_ARGB_R(rgb);
        green += GET_ARGB_G(rgb);
        blue += GET_ARGB_B(rgb);

        rgb = r0[x + r1];
        red += GET_ARGB_R(rgb);
        green += GET_ARGB_G(rgb);
        blue += GET_ARGB_B(rgb);

        rgb = r0[x + r2];
        red += GET_ARGB_R(rgb);
        green += GET_ARGB_G(rgb);
        blue += GET_ARGB_B(rgb);

        rgb = rp1[x - l1];
        red += GET_ARGB_R(rgb);
        green += GET_ARGB_G(rgb);
        blue += GET_ARGB_B(rgb);

        rgb = rp1[x];
        red += GET_ARGB_R(rgb);
        green += GET_ARGB_G(rgb);
        blue += GET_ARGB_B(rgb);

        rgb = rp1[x + r1];
        red += GET_ARGB_R(rgb);
        green += GET_ARGB_G(rgb);
        blue += GET_ARGB_B(rgb);

        rgb = rp2[x];
        red += GET_ARGB_R(rgb);
        green += GET_ARGB_G(rgb);
        blue += GET_ARGB_B(rgb);

        red = red / 14;
        green = green / 14;
        blue = blue / 14;

        if (tex)
        {
            argb = r0[x];
            as = 255 - GET_ARGB_A(tex[x]);
            rs = GET_ARGB_R(argb);
            gs = GET_ARGB_G(argb);
            bs = GET_ARGB_B(argb);

            red = ((rs * as) >> 8) + ((red * (255 - as)) >> 8);
            green = ((gs * as) >> 8) + ((green * (255 - as)) >> 8);
            blue = ((bs * as) >> 8) + ((blue * (255 - as)) >> 8);

            d[x] = SET_ARGB(as, red, green, blue);
        }
        else
        {
            d[x] = SET_ARGB(255, red, green, blue);
        }
    }
}
