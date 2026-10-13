#ifndef WRITEPIXELARRAYALPHA_SIMD_H
#define WRITEPIXELARRAYALPHA_SIMD_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Shared pieces of the x86-64 WritePixelArrayAlpha() kernels.

    All pixels handled here are ARGB32 in memory order: the bytes A, R, G, B.
    Read as a little endian 32-bit value that puts alpha in bits 0..7.
*/

#include <exec/types.h>

/*
 * Blend 'width' source pixels over the destination row.
 *   galpha   - global alpha, scales every source pixel's own alpha (255 = unchanged)
 *   dstalpha - the destination has an alpha channel that takes part in the
 *              blend and is updated; otherwise it is opaque
 */
typedef void (*wpaa_blendrow_t)(UBYTE *dst, const UBYTE *src, ULONG width, UBYTE galpha, BOOL dstalpha);

/* Copy 'width' pixels from src to dst with their alpha scaled by galpha */
typedef void (*wpaa_scalerow_t)(UBYTE *dst, const UBYTE *src, ULONG width, UBYTE galpha);

/* The AVX2 kernels, in writepixelarrayalpha_avx.c */
void WritePixelArrayAlpha_BlendRow_AVX(UBYTE *dst, const UBYTE *src, ULONG width, UBYTE galpha, BOOL dstalpha);
void WritePixelArrayAlpha_ScaleRow_AVX(UBYTE *dst, const UBYTE *src, ULONG width, UBYTE galpha);

/* round(v / 255) for v in 0..65025 */
static inline ULONG wpaa_div255(ULONG v)
{
    v += 128;
    return (v + (v >> 8)) >> 8;
}

/* Source alpha scaled by the global alpha; same rounding as the generic code */
static inline ULONG wpaa_scale_alpha(ULONG a, ULONG galpha)
{
    ULONG v = a * galpha + 127;

    return (v + (v >> 8)) >> 8;
}

/*
 * One pixel, all cases. The vector loops use it for the pixels left over
 * at the end of a row and for groups whose destination is translucent.
 */
static inline void wpaa_blend_pixel(UBYTE *d, const UBYTE *s, UBYTE galpha, BOOL dstalpha)
{
    ULONG sa = wpaa_scale_alpha(s[0], galpha);
    ULONG da;

    if (sa == 0)
        return;

    da = dstalpha ? d[0] : 0xFF;

    if (da == 0xFF)
    {
        /* Opaque destination: interpolate, alpha stays full */
        ULONG ia = 255 - sa;

        d[0] = 0xFF;
        d[1] = wpaa_div255(s[1] * sa + d[1] * ia);
        d[2] = wpaa_div255(s[2] * sa + d[2] * ia);
        d[3] = wpaa_div255(s[3] * sa + d[3] * ia);
    }
    else
    {
        /* Translucent destination: full non-premultiplied source-over */
        ULONG oa  = sa + ((da * (255 - sa) + 127) / 255);
        ULONG div = oa * 255;
        ULONG dw  = da * (255 - sa);

        d[0] = oa;
        d[1] = (s[1] * sa * 255 + d[1] * dw + div / 2) / div;
        d[2] = (s[2] * sa * 255 + d[2] * dw + div / 2) / div;
        d[3] = (s[3] * sa * 255 + d[3] * dw + div / 2) / div;
    }
}

static inline void wpaa_scale_pixel(UBYTE *d, const UBYTE *s, UBYTE galpha)
{
    d[0] = wpaa_scale_alpha(s[0], galpha);
    d[1] = s[1];
    d[2] = s[2];
    d[3] = s[3];
}

#endif /* WRITEPIXELARRAYALPHA_SIMD_H */
