/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: AVX2 row kernels for cybergraphics.library/WritePixelArrayAlpha().
          Only reached when writepixelarrayalpha.c found AVX2 usable.
*/

#include <exec/types.h>

#ifdef __clang__
_Pragma("clang attribute push(__attribute__((target(\"avx2\"))), apply_to=function)")
#elif defined(__GNUC__)
_Pragma("GCC push_options")
_Pragma("GCC target(\"avx2\")")
#endif

#include <immintrin.h>

#include "writepixelarrayalpha_simd.h"

/*
 * Blend eight ARGB32 pixels at a time. See the SSE2 version in
 * writepixelarrayalpha.c for the arithmetic; this is the same thing on
 * 256-bit registers. unpack/shuffle/pack all work within each 128-bit
 * half, so the pixel order is preserved.
 */
void WritePixelArrayAlpha_BlendRow_AVX(UBYTE *dst, const UBYTE *src, ULONG width, UBYTE galpha, BOOL dstalpha)
{
    const __m256i zero  = _mm256_setzero_si256();
    const __m256i vga   = _mm256_set1_epi16(galpha);
    const __m256i c127  = _mm256_set1_epi16(127);
    const __m256i c128  = _mm256_set1_epi16(128);
    const __m256i c255  = _mm256_set1_epi16(255);
    const __m256i amask = _mm256_set1_epi32(0x000000FF);
    const BOOL    noscale = (galpha == 0xFF);
    ULONG x = 0;

    for (; x + 8 <= width; x += 8)
    {
        __m256i s = _mm256_loadu_si256((const __m256i *)(src + x * 4));
        __m256i sa = _mm256_and_si256(s, amask);
        __m256i d, s16, d16, a, ia, t, lo, hi;

        /* Nothing to do where every source pixel is fully transparent */
        if (_mm256_movemask_epi8(_mm256_cmpeq_epi32(sa, zero)) == -1)
            continue;

        /* Fully opaque source pixels simply replace the destination */
        if (noscale && (_mm256_movemask_epi8(_mm256_cmpeq_epi32(sa, amask)) == -1))
        {
            _mm256_storeu_si256((__m256i *)(dst + x * 4), s);
            continue;
        }

        d = _mm256_loadu_si256((const __m256i *)(dst + x * 4));

        /* A translucent destination needs a division per pixel: leave it to the scalar code */
        if (dstalpha &&
            (_mm256_movemask_epi8(_mm256_cmpeq_epi32(_mm256_and_si256(d, amask), amask)) != -1))
        {
            ULONG i;

            for (i = 0; i < 8; i++)
                wpaa_blend_pixel(dst + (x + i) * 4, src + (x + i) * 4, galpha, dstalpha);
            continue;
        }

        /* Pixels 0,1 and 4,5 */
        s16 = _mm256_unpacklo_epi8(s, zero);
        d16 = _mm256_unpacklo_epi8(d, zero);
        a   = _mm256_shufflehi_epi16(_mm256_shufflelo_epi16(s16, 0), 0);
        t   = _mm256_add_epi16(_mm256_mullo_epi16(a, vga), c127);
        a   = _mm256_srli_epi16(_mm256_add_epi16(t, _mm256_srli_epi16(t, 8)), 8);
        ia  = _mm256_sub_epi16(c255, a);
        t   = _mm256_add_epi16(_mm256_add_epi16(_mm256_mullo_epi16(s16, a), _mm256_mullo_epi16(d16, ia)), c128);
        lo  = _mm256_srli_epi16(_mm256_add_epi16(t, _mm256_srli_epi16(t, 8)), 8);

        /* Pixels 2,3 and 6,7 */
        s16 = _mm256_unpackhi_epi8(s, zero);
        d16 = _mm256_unpackhi_epi8(d, zero);
        a   = _mm256_shufflehi_epi16(_mm256_shufflelo_epi16(s16, 0), 0);
        t   = _mm256_add_epi16(_mm256_mullo_epi16(a, vga), c127);
        a   = _mm256_srli_epi16(_mm256_add_epi16(t, _mm256_srli_epi16(t, 8)), 8);
        ia  = _mm256_sub_epi16(c255, a);
        t   = _mm256_add_epi16(_mm256_add_epi16(_mm256_mullo_epi16(s16, a), _mm256_mullo_epi16(d16, ia)), c128);
        hi  = _mm256_srli_epi16(_mm256_add_epi16(t, _mm256_srli_epi16(t, 8)), 8);

        /* The destination is opaque here, and stays so */
        _mm256_storeu_si256((__m256i *)(dst + x * 4),
                            _mm256_or_si256(_mm256_packus_epi16(lo, hi), amask));
    }

    _mm256_zeroupper();

    for (; x < width; x++)
        wpaa_blend_pixel(dst + x * 4, src + x * 4, galpha, dstalpha);
}

/* Copy eight pixels at a time, scaling the alpha byte of each */
void WritePixelArrayAlpha_ScaleRow_AVX(UBYTE *dst, const UBYTE *src, ULONG width, UBYTE galpha)
{
    const __m256i vga   = _mm256_set1_epi32(galpha);
    const __m256i c127  = _mm256_set1_epi32(127);
    const __m256i amask = _mm256_set1_epi32(0x000000FF);
    ULONG x = 0;

    for (; x + 8 <= width; x += 8)
    {
        __m256i s = _mm256_loadu_si256((const __m256i *)(src + x * 4));
        __m256i a = _mm256_and_si256(s, amask);
        __m256i v = _mm256_add_epi32(_mm256_mullo_epi16(a, vga), c127);

        a = _mm256_srli_epi32(_mm256_add_epi32(v, _mm256_srli_epi32(v, 8)), 8);

        _mm256_storeu_si256((__m256i *)(dst + x * 4),
                            _mm256_or_si256(_mm256_andnot_si256(amask, s), a));
    }

    _mm256_zeroupper();

    for (; x < width; x++)
        wpaa_scale_pixel(dst + x * 4, src + x * 4, galpha);
}

#ifdef __clang__
_Pragma("clang attribute pop")
#elif defined(__GNUC__)
_Pragma("GCC pop_options")
#endif
