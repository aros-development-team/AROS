/*
    Copyright (C) 2026, The AROS Development Team.

    Desc: decorator.library - row-level pixel kernels, x86_64 SSE2 implementations

    SSE2 is part of the x86_64 ABI baseline, so no runtime feature
    detection is needed. These kernels must produce output identical to
    the generic C implementations in workbench/libs/decorator/pixops.c.

    Pixel data is big-endian ARGB read as native (little endian) ULONGs,
    so the in-register byte order per pixel is A,R,G,B from the lowest
    byte upwards. After unpacking with zero the 16bit lane order per
    pixel is [A R G B].
*/

#include <exec/types.h>

#include <libraries/decorator.h>
#include "decorator_intern.h"

#include <emmintrin.h>

void pixop_splat_row(ULONG *d, LONG n, ULONG px)
{
    __m128i v = _mm_set1_epi32(px);
    LONG i = 0;

    for (; i + 4 <= n; i += 4)
        _mm_storeu_si128((__m128i *)(d + i), v);
    for (; i < n; i++)
        d[i] = px;
}

void pixop_extract_alpha_row(UBYTE *d, const ULONG *s, LONG n)
{
    const __m128i amask = _mm_set1_epi32(0xFF);
    LONG i = 0;

    for (; i + 16 <= n; i += 16)
    {
        __m128i a0 = _mm_and_si128(_mm_loadu_si128((const __m128i *)(s + i)), amask);
        __m128i a1 = _mm_and_si128(_mm_loadu_si128((const __m128i *)(s + i + 4)), amask);
        __m128i a2 = _mm_and_si128(_mm_loadu_si128((const __m128i *)(s + i + 8)), amask);
        __m128i a3 = _mm_and_si128(_mm_loadu_si128((const __m128i *)(s + i + 12)), amask);

        _mm_storeu_si128((__m128i *)(d + i),
            _mm_packus_epi16(_mm_packs_epi32(a0, a1), _mm_packs_epi32(a2, a3)));
    }
    for (; i < n; i++)
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

    const __m128i zero = _mm_setzero_si128();
    const __m128i invv = _mm_set1_epi16(inv);
    /* per pixel 16bit lanes are [A R G B]; alpha is forced below */
    const __m128i addv = _mm_set_epi16(cb, cg, cr, 0, cb, cg, cr, 0);
    const __m128i amask = _mm_set1_epi32(0xFF);
    const __m128i rgbmask = _mm_set1_epi32(0xFFFFFF00);
    LONG i = 0;

    for (; i + 4 <= n; i += 4)
    {
        __m128i px = _mm_loadu_si128((const __m128i *)(d + i));
        __m128i lo = _mm_unpacklo_epi8(px, zero);
        __m128i hi = _mm_unpackhi_epi8(px, zero);

        lo = _mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(lo, invv), 8), addv);
        hi = _mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(hi, invv), 8), addv);

        __m128i out = _mm_packus_epi16(lo, hi);
        out = _mm_or_si128(_mm_and_si128(out, rgbmask), amask);
        _mm_storeu_si128((__m128i *)(d + i), out);
    }

    for (; i < n; i++)
    {
        ULONG rgb = d[i];
        UWORD r = cr + ((GET_ARGB_R(rgb) * inv) >> 8);
        UWORD g = cg + ((GET_ARGB_G(rgb) * inv) >> 8);
        UWORD b = cb + ((GET_ARGB_B(rgb) * inv) >> 8);
        d[i] = SET_ARGB(255, r, g, b);
    }
}

void pixop_mix_row(ULONG *d, const ULONG *s, LONG n, UWORD ratio, BOOL tiled)
{
    UWORD   inv = 255 - ratio;

    const __m128i zero = _mm_setzero_si128();
    const __m128i rv = _mm_set1_epi16(ratio);
    const __m128i invv = _mm_set1_epi16(inv);
    const __m128i c255 = _mm_set1_epi16(255);
    const __m128i amask = _mm_set1_epi32(0xFF);
    const __m128i rgbmask = _mm_set1_epi32(0xFFFFFF00);
    LONG i = 0;

    for (; i + 4 <= n; i += 4)
    {
        __m128i sp = _mm_loadu_si128((const __m128i *)(s + i));
        __m128i dp = _mm_loadu_si128((const __m128i *)(d + i));
        __m128i slo = _mm_unpacklo_epi8(sp, zero);
        __m128i shi = _mm_unpackhi_epi8(sp, zero);
        __m128i dlo = _mm_unpacklo_epi8(dp, zero);
        __m128i dhi = _mm_unpackhi_epi8(dp, zero);

        __m128i mlo = _mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(slo, rv), 8),
                                    _mm_srli_epi16(_mm_mullo_epi16(dlo, invv), 8));
        __m128i mhi = _mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(shi, rv), 8),
                                    _mm_srli_epi16(_mm_mullo_epi16(dhi, invv), 8));

        if (tiled)
        {
            /* broadcast the source alpha lane over each pixel's lanes */
            __m128i aslo = _mm_shufflehi_epi16(_mm_shufflelo_epi16(slo, 0), 0);
            __m128i ashi = _mm_shufflehi_epi16(_mm_shufflelo_epi16(shi, 0), 0);
            __m128i ialo = _mm_sub_epi16(c255, aslo);
            __m128i iahi = _mm_sub_epi16(c255, ashi);

            mlo = _mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(mlo, aslo), 8),
                                _mm_srli_epi16(_mm_mullo_epi16(dlo, ialo), 8));
            mhi = _mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(mhi, ashi), 8),
                                _mm_srli_epi16(_mm_mullo_epi16(dhi, iahi), 8));
        }

        __m128i out = _mm_packus_epi16(mlo, mhi);
        /* result alpha comes from the source pixel */
        out = _mm_or_si128(_mm_and_si128(out, rgbmask), _mm_and_si128(sp, amask));
        _mm_storeu_si128((__m128i *)(d + i), out);
    }

    for (; i < n; i++)
    {
        ULONG rgba = s[i];
        ULONG rgb = d[i];
        UBYTE as = GET_ARGB_A(rgba);
        UBYTE rd = GET_ARGB_R(rgb), gd = GET_ARGB_G(rgb), bd = GET_ARGB_B(rgb);
        UWORD r = ((GET_ARGB_R(rgba) * ratio) >> 8) + ((rd * inv) >> 8);
        UWORD g = ((GET_ARGB_G(rgba) * ratio) >> 8) + ((gd * inv) >> 8);
        UWORD b = ((GET_ARGB_B(rgba) * ratio) >> 8) + ((bd * inv) >> 8);

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
    const __m128i zero = _mm_setzero_si128();
    const __m128i fv = _mm_set1_epi16(fact);
    const __m128i c255 = _mm_set1_epi16(255);
    LONG i = 0;

    for (; i + 4 <= n; i += 4)
    {
        __m128i px = _mm_loadu_si128((const __m128i *)(s + i));
        __m128i lo = _mm_unpacklo_epi8(px, zero);
        __m128i hi = _mm_unpackhi_epi8(px, zero);

        /* (c * fact) >> 8 built from the low and high halves of the
           32bit product; the product is below 2^24 so the shifted
           result always fits in a 16bit lane */
        __m128i lolo = _mm_mullo_epi16(lo, fv);
        __m128i lohi = _mm_mulhi_epu16(lo, fv);
        __m128i hilo = _mm_mullo_epi16(hi, fv);
        __m128i hihi = _mm_mulhi_epu16(hi, fv);

        lo = _mm_or_si128(_mm_srli_epi16(lolo, 8), _mm_slli_epi16(lohi, 8));
        hi = _mm_or_si128(_mm_srli_epi16(hilo, 8), _mm_slli_epi16(hihi, 8));

        /* unsigned min(x, 255): packus alone saturates signed lanes, which
           would turn values above 0x7FFF into 0 instead of 255 */
        lo = _mm_sub_epi16(lo, _mm_subs_epu16(lo, c255));
        hi = _mm_sub_epi16(hi, _mm_subs_epu16(hi, c255));

        _mm_storeu_si128((__m128i *)(d + i), _mm_packus_epi16(lo, hi));
    }

    for (; i < n; i++)
    {
        ULONG rgb = s[i];
        ULONG c0 = (GET_ARGB_A(rgb) * fact) >> 8;
        ULONG c1 = (GET_ARGB_R(rgb) * fact) >> 8;
        ULONG c2 = (GET_ARGB_G(rgb) * fact) >> 8;
        ULONG c3 = (GET_ARGB_B(rgb) * fact) >> 8;
        if (c0 > 255) c0 = 255;
        if (c1 > 255) c1 = 255;
        if (c2 > 255) c2 = 255;
        if (c3 > 255) c3 = 255;
        d[i] = SET_ARGB(c0, c1, c2, c3);
    }
}

/* Scalar blur for the row edges and vector tail; identical to the
   generic implementation */
static void blur14mix_scalar(ULONG *d, const ULONG *rm2, const ULONG *rm1, const ULONG *r0,
                             const ULONG *rp1, const ULONG *rp2, const ULONG *tex,
                             LONG w, LONG x0, LONG x1)
{
    LONG    x, l1, l2, r1, r2;
    ULONG   rgb, argb;
    UWORD   red, green, blue, as, rs, gs, bs;

    for (x = x0; x < x1; x++)
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

void pixop_blur14mix_row(ULONG *d, const ULONG *rm2, const ULONG *rm1, const ULONG *r0,
                         const ULONG *rp1, const ULONG *rp2, const ULONG *tex, LONG w)
{
    const __m128i zero = _mm_setzero_si128();
    /* (sum * 4682) >> 16 == sum / 14 for all sums up to 14 * 255 */
    const __m128i div14 = _mm_set1_epi16(4682);
    const __m128i c255 = _mm_set1_epi16(255);
    const __m128i amask = _mm_set1_epi32(0xFF);
    const __m128i rgbmask = _mm_set1_epi32(0xFFFFFF00);
    LONG i;

    if (w < 8)
    {
        blur14mix_scalar(d, rm2, rm1, r0, rp1, rp2, tex, w, 0, w);
        return;
    }

    /* Left edge (x < 2) scalar */
    blur14mix_scalar(d, rm2, rm1, r0, rp1, rp2, tex, w, 0, 2);

    /* Middle: all taps are unclamped for 2 <= x < w - 2 */
    i = 2;
    for (; i + 4 <= w - 2; i += 4)
    {
        __m128i acclo, acchi, t;

#define ADDTAP(ptr, off)                                                    \
        t = _mm_loadu_si128((const __m128i *)((ptr) + i + (off)));          \
        acclo = _mm_add_epi16(acclo, _mm_unpacklo_epi8(t, zero));           \
        acchi = _mm_add_epi16(acchi, _mm_unpackhi_epi8(t, zero));

        t = _mm_loadu_si128((const __m128i *)(r0 + i));
        acclo = _mm_unpacklo_epi8(t, zero);
        acchi = _mm_unpackhi_epi8(t, zero);

        ADDTAP(rm2, 0)
        ADDTAP(rm1, -1)
        ADDTAP(rm1, 0)
        ADDTAP(rm1, 0)
        ADDTAP(rm1, 1)
        ADDTAP(r0, -2)
        ADDTAP(r0, -1)
        ADDTAP(r0, 1)
        ADDTAP(r0, 2)
        ADDTAP(rp1, -1)
        ADDTAP(rp1, 0)
        ADDTAP(rp1, 1)
        ADDTAP(rp2, 0)
#undef ADDTAP

        __m128i blo = _mm_mulhi_epu16(acclo, div14);
        __m128i bhi = _mm_mulhi_epu16(acchi, div14);
        __m128i out;

        if (tex)
        {
            __m128i tx = _mm_loadu_si128((const __m128i *)(tex + i));
            __m128i txlo = _mm_unpacklo_epi8(tx, zero);
            __m128i txhi = _mm_unpackhi_epi8(tx, zero);
            /* as = 255 - texture alpha, broadcast over each pixel's lanes */
            __m128i aslo = _mm_sub_epi16(c255, _mm_shufflehi_epi16(_mm_shufflelo_epi16(txlo, 0), 0));
            __m128i ashi = _mm_sub_epi16(c255, _mm_shufflehi_epi16(_mm_shufflelo_epi16(txhi, 0), 0));
            __m128i ialo = _mm_sub_epi16(c255, aslo);
            __m128i iahi = _mm_sub_epi16(c255, ashi);

            __m128i o = _mm_loadu_si128((const __m128i *)(r0 + i));
            __m128i olo = _mm_unpacklo_epi8(o, zero);
            __m128i ohi = _mm_unpackhi_epi8(o, zero);

            blo = _mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(olo, aslo), 8),
                                _mm_srli_epi16(_mm_mullo_epi16(blo, ialo), 8));
            bhi = _mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(ohi, ashi), 8),
                                _mm_srli_epi16(_mm_mullo_epi16(bhi, iahi), 8));

            out = _mm_packus_epi16(blo, bhi);
            /* result alpha = 255 - texture alpha */
            out = _mm_or_si128(_mm_and_si128(out, rgbmask),
                               _mm_and_si128(_mm_xor_si128(tx, amask), amask));
        }
        else
        {
            out = _mm_packus_epi16(blo, bhi);
            out = _mm_or_si128(_mm_and_si128(out, rgbmask), amask);
        }

        _mm_storeu_si128((__m128i *)(d + i), out);
    }

    /* Right edge and vector remainder scalar */
    blur14mix_scalar(d, rm2, rm1, r0, rp1, rp2, tex, w, i, w);
}
