/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.
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

/* c * t / 255, rounded to nearest, for 0 <= c, t <= 255 */
static inline UBYTE mul255(UBYTE c, UBYTE t)
{
    ULONG v = (ULONG)c * t + 128;

    return (v + (v >> 8)) >> 8;
}

/*
 * RECTFMT_ARGB is a byte order (A, R, G, B in memory): a pixel loaded as an
 * x86 32-bit word has alpha in its low byte, then red, green, blue.
 */
static inline void TintRow_SSE2(ULONG *p, int n, ULONG tint)
{
    UBYTE tr = (tint >> 16) & 0xFF, tg = (tint >> 8) & 0xFF, tb = tint & 0xFF;
    const __m128i maskA   = _mm_set1_epi32(0x000000FFu);         // keep alpha
    const __m128i zero    = _mm_setzero_si128();
    const __m128i tint32  = _mm_set1_epi32(((ULONG)tr << 8) | ((ULONG)tg << 16) | ((ULONG)tb << 24));

    int i = 0;

#if defined(TINT_MODE_MULTIPLY)
    // Multiplicative: (c * t) / 255 with rounding, per channel on RGB; keep alpha
    const __m128i t_lo   = _mm_unpacklo_epi8(tint32, zero);
    const __m128i t_hi   = _mm_unpackhi_epi8(tint32, zero);
    const __m128i add128 = _mm_set1_epi16(128);

    for (; i + 4 <= n; i += 4) {
        __m128i pix    = _mm_loadu_si128((const __m128i*)(p + i));
        __m128i alpha  = _mm_and_si128(pix, maskA);

        __m128i m_lo   = _mm_mullo_epi16(_mm_unpacklo_epi8(pix, zero), t_lo);
        __m128i m_hi   = _mm_mullo_epi16(_mm_unpackhi_epi8(pix, zero), t_hi);

        m_lo = _mm_add_epi16(m_lo, add128);
        m_hi = _mm_add_epi16(m_hi, add128);
        m_lo = _mm_srli_epi16(_mm_add_epi16(m_lo, _mm_srli_epi16(m_lo, 8)), 8);
        m_hi = _mm_srli_epi16(_mm_add_epi16(m_hi, _mm_srli_epi16(m_hi, 8)), 8);

        __m128i mul_rgb = _mm_packus_epi16(m_lo, m_hi);

        __m128i out     = _mm_or_si128(alpha, _mm_andnot_si128(maskA, mul_rgb));
        _mm_storeu_si128((__m128i*)(p + i), out);
    }
#else
    // Additive: per-byte saturating add on RGB (0 in the alpha byte), keep alpha
    for (; i + 4 <= n; i += 4) {
        __m128i pix      = _mm_loadu_si128((const __m128i*)(p + i));
        _mm_storeu_si128((__m128i*)(p + i), _mm_adds_epu8(pix, tint32));
    }
#endif

    // Tail (scalar), on the bytes
    for (; i < n; ++i) {
        UBYTE *px = (UBYTE *)(p + i);

#if defined(TINT_MODE_MULTIPLY)
        px[1] = mul255(px[1], tr);
        px[2] = mul255(px[2], tg);
        px[3] = mul255(px[3], tb);
#else
        UWORD rr = px[1] + tr; px[1] = (rr > 255) ? 255 : rr;
        UWORD gg = px[2] + tg; px[2] = (gg > 255) ? 255 : gg;
        UWORD bb = px[3] + tb; px[3] = (bb > 255) ? 255 : bb;
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
