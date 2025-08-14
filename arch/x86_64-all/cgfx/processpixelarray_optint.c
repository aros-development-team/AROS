/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <aros/debug.h>

#include <proto/cybergraphics.h>

#include <hidd/gfx.h>
#include <cybergraphx/cybergraphics.h>
#include <exec/types.h>

#include "cybergraphics_intern.h"

#define TINT_MODE_MULTIPLY

#include <emmintrin.h>  // SSE2
#include <stdint.h>

static inline void TintRow_SSE2(ULONG *p, int n, ULONG tint)
{
    const __m128i maskA   = _mm_set1_epi32(0xFF000000u);          // keep alpha
    const __m128i zero    = _mm_setzero_si128();
    const __m128i tint32  = _mm_set1_epi32(tint);
    const __m128i tintRGB = _mm_andnot_si128(maskA, tint32);      // RGB only

    int i = 0;

#if defined(TINT_MODE_MULTIPLY)
    // Multiplicative: (c * t) / 255 with rounding, per channel on RGB; keep alpha
    for (; i + 4 <= n; i += 4) {
        __m128i pix    = _mm_loadu_si128((const __m128i*)(p + i));
        __m128i alpha  = _mm_and_si128(pix, maskA);

        __m128i p_lo   = _mm_unpacklo_epi8(pix,    zero);
        __m128i p_hi   = _mm_unpackhi_epi8(pix,    zero);
        __m128i t_lo   = _mm_unpacklo_epi8(tint32, zero);
        __m128i t_hi   = _mm_unpackhi_epi8(tint32, zero);

        __m128i m_lo   = _mm_mullo_epi16(p_lo, t_lo);
        __m128i m_hi   = _mm_mullo_epi16(p_hi, t_hi);

        __m128i add128 = _mm_set1_epi16(128);
        m_lo = _mm_add_epi16(m_lo, add128);
        m_hi = _mm_add_epi16(m_hi, add128);
        __m128i lo_sh  = _mm_srli_epi16(m_lo, 8);
        __m128i hi_sh  = _mm_srli_epi16(m_hi, 8);
        m_lo = _mm_add_epi16(m_lo, lo_sh);
        m_hi = _mm_add_epi16(m_hi, hi_sh);
        m_lo = _mm_srli_epi16(m_lo, 8);
        m_hi = _mm_srli_epi16(m_hi, 8);

        __m128i mul_rgb = _mm_packus_epi16(m_lo, m_hi);

        __m128i out     = _mm_or_si128(alpha, _mm_andnot_si128(maskA, mul_rgb));
        _mm_storeu_si128((__m128i*)(p + i), out);
    }
#else
#    // Additive: per-byte saturating add on RGB, keep alpha
    for (; i + 4 <= n; i += 4) {
        __m128i pix      = _mm_loadu_si128((const __m128i*)(p + i));
        __m128i alpha    = _mm_and_si128(pix, maskA);
        __m128i rgb      = _mm_andnot_si128(maskA, pix);
        __m128i out_rgb  = _mm_adds_epu8(rgb, tintRGB);
        __m128i out      = _mm_or_si128(alpha, out_rgb);
        _mm_storeu_si128((__m128i*)(p + i), out);
    }
#endif

    // Tail (scalar)
    for (; i < n; ++i) {
        ULONG px = p[i];
        UBYTE a = (px >> 24) & 0xFF, r = (px >> 16) & 0xFF, g = (px >> 8) & 0xFF, b = px & 0xFF;
        UBYTE tr = (tint >> 16) & 0xFF, tg = (tint >> 8) & 0xFF, tb = tint & 0xFF;

#if defined(TINT_MODE_MULTIPLY)
        p[i] = (ULONG)a<<24 |
               (ULONG)((r * tr + 128 + ((r * tr + 128) >> 8)) >> 8) << 16 |
               (ULONG)((g * tg + 128 + ((g * tg + 128) >> 8)) >> 8) << 8  |
               (ULONG)((b * tb + 128 + ((b * tb + 128) >> 8)) >> 8);        
#else
        UWORD rr = r + tr; if (rr > 255) rr = 255;
        UWORD gg = g + tg; if (gg > 255) gg = 255;
        UWORD bb = b + tb; if (bb > 255) bb = 255;
        p[i] = (ULONG)a<<24 | (ULONG)rr<<16 | (ULONG)gg<<8 | (ULONG)bb;
#endif
    }
}

void ProcessPixelArrayTintFunc(struct RastPort *opRast, struct Rectangle *opRect, ULONG tint, struct Library *CyberGfxBase)
{
    const ULONG w = opRect->MaxX - opRect->MinX + 1;
    const ULONG h = opRect->MaxY - opRect->MinY + 1;

    ULONG *buf = (ULONG *)AllocVec(w * h * 4, MEMF_ANY);
    if (buf) {
        int y;
        ReadPixelArray(buf, 0, 0, w * 4, opRast, opRect->MinX, opRect->MinY, w, h, RECTFMT_ARGB);
        for (y = 0; y < h; ++y)
            TintRow_SSE2(buf + y * w, w, tint);
        WritePixelArray(buf, 0, 0, w * 4, opRast, opRect->MinX, opRect->MinY, w, h, RECTFMT_ARGB);
        FreeVec(buf);
    }
}
