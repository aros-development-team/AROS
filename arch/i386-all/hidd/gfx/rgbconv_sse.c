/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#if defined(__SSE__)
#define DEBUG 0
#include <aros/debug.h>

#include <exec/types.h>
#include <hidd/gfx.h>

#include <emmintrin.h>
#include <x86intrin.h>

#include "colorconv/rgbconv_macros.h"

#if defined ARCHCONVERTFUNCH
#undef ARCHCONVERTFUNCH
#endif
#define ARCHCONVERTFUNCH(arch, a, b) \
ULONG convert_ ## a ## _ ## b ## _ ## arch \
    (APTR srcPixels, ULONG srcMod, HIDDT_StdPixFmt srcPixFmt, \
    APTR dstPixels, ULONG dstMod, HIDDT_StdPixFmt dstPixFmt, \
    UWORD width, UWORD height) \
{

// Macro to convert one register of 4 pixels from ABCD to DCBA
#if defined REVERSECMPNTS_4PX
#undef REVERSECMPNTS_4PX
#endif
#define REVERSECMPNTS_4PX(px)                                         \
    __m128i pa_##px = _mm_and_si128(_mm_srli_epi32(px, 24), mask_ff); \
    __m128i pb_##px = _mm_and_si128(_mm_srli_epi32(px, 16), mask_ff); \
    __m128i pc_##px = _mm_and_si128(_mm_srli_epi32(px, 8),  mask_ff); \
    __m128i pd_##px = _mm_and_si128(px, mask_ff);                    \
    __m128i pba_##px = _mm_or_si128(pa_##px, _mm_slli_epi32(pb_##px, 8)); \
    __m128i pcba_##px = _mm_or_si128(pba_##px, _mm_slli_epi32(pc_##px, 16)); \
    px = _mm_or_si128(pcba_##px, _mm_slli_epi32(pd_##px, 24))

#if defined SSE_ABCDtoDCBA
#undef SSE_ABCDtoDCBA
#endif
#define SSE_ABCDtoDCBA                                         \
            __m128i p0 = _mm_loadu_si128((const __m128i*)&src[x]); \
            __m128i p1 = _mm_loadu_si128((const __m128i*)&src[x + 4]); \
            __m128i p2 = _mm_loadu_si128((const __m128i*)&src[x + 8]); \
            __m128i p3 = _mm_loadu_si128((const __m128i*)&src[x + 12]); \
            REVERSECMPNTS_4PX(p0); \
            REVERSECMPNTS_4PX(p1); \
            REVERSECMPNTS_4PX(p2); \
            REVERSECMPNTS_4PX(p3); \
            _mm_storeu_si128((__m128i*)&dst[x],  p0); \
            _mm_storeu_si128((__m128i*)&dst[x + 4],  p1); \
            _mm_storeu_si128((__m128i*)&dst[x + 8],  p2); \
            _mm_storeu_si128((__m128i*)&dst[x + 12], p3)

/* Opearations work on 16 pixels at a time */
#define PIXELSPERCONV (4 << 2)

ARCHCONVERTFUNCH(SSE2,XRGB32,BGRA32)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG y = 0;

    const __m128i mask_ff = _mm_set1_epi32(0xFF);

    D(bug("[GFX:SSE] %s()\n", __func__);)

    for (; y < height; ++y) {
        ULONG x = 0;

        for (; x + PIXELSPERCONV <= width; x += PIXELSPERCONV) {
            // Load 16 pixels = 4 x 128-bit SSE2 register loads, Convert registers, and write them out
            SSE_ABCDtoDCBA;
        }

        // Handle remaining pixels
        for (; x < width; ++x) {
            ULONG s = src[x];
            dst[x] = SHUFFLE24(s, ARGB32, BGRA32);
        }

        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}

ARCHCONVERTFUNCH(SSE2,BGRA32,XRGB32)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG y = 0;

    const __m128i mask_ff = _mm_set1_epi32(0xFF);

    D(bug("[GFX:SSE] %s()\n", __func__);)

    for (; y < height; ++y)
    {
        ULONG x = 0;
        for (; x + PIXELSPERCONV < width; x += PIXELSPERCONV)
        {
            // Load 16 pixels = 4 x 128-bit SSE2 register loads, Convert registers, and write them out
            SSE_ABCDtoDCBA;
        }

        // Handle remaining pixels
        for (; x < width; ++x)
        {
            ULONG s = src[x];
            dst[x] = SHUFFLE24(s, BGRA32, ARGB32);
        }

        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}

#if defined(__SSSE3__)
#define SSE_ABCDtoDCBA_MASK \
        0x0C, 0x0D, 0x0E, 0x0F, \
        0x08, 0x09, 0x0A, 0x0B, \
        0x04, 0x05, 0x06, 0x07, \
        0x00, 0x01, 0x02, 0x03
ARCHCONVERTFUNCH(SSE3,XRGB32,BGRA32)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG y = 0;

    const __m128i shuffle_mask = _mm_set_epi8(
        SSE_ABCDtoDCBA_MASK
    );

    D(bug("[GFX:SSE] %s()\n", __func__);)

    for (; y < height; ++y) {
        ULONG x = 0;
        for (; x + PIXELSPERCONV <= width; x += PIXELSPERCONV) {
            __m128i p0 = _mm_loadu_si128((const __m128i*)&src[x]);
            __m128i p1 = _mm_loadu_si128((const __m128i*)&src[x + 4]);
            __m128i p2 = _mm_loadu_si128((const __m128i*)&src[x + 8]);
            __m128i p3 = _mm_loadu_si128((const __m128i*)&src[x + 12]);

            // Reorder bytes using shuffle
            _mm_storeu_si128((__m128i*)&dst[x],  _mm_shuffle_epi8(p0, shuffle_mask));
            _mm_storeu_si128((__m128i*)&dst[x + 4],  _mm_shuffle_epi8(p1, shuffle_mask));
            _mm_storeu_si128((__m128i*)&dst[x + 8],  _mm_shuffle_epi8(p2, shuffle_mask));
            _mm_storeu_si128((__m128i*)&dst[x + 12], _mm_shuffle_epi8(p3, shuffle_mask));
        }

        // Handle remaining pixels
        for (; x < width; ++x) {
            ULONG s = src[x];
            dst[x] = SHUFFLE24(s, ARGB32, BGRA32);
        }

        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}

ARCHCONVERTFUNCH(SSE3,BGRA32,XRGB32)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG y = 0;

    const __m128i shuffle_mask = _mm_set_epi8(
        SSE_ABCDtoDCBA_MASK
    );

    D(bug("[GFX:SSE] %s()\n", __func__);)

    for (; y < height; ++y) {
        ULONG x = 0;
        for (; x + PIXELSPERCONV <= width; x += PIXELSPERCONV) {
            __m128i p0 = _mm_loadu_si128((const __m128i*)&src[x]);
            __m128i p1 = _mm_loadu_si128((const __m128i*)&src[x + 4]);
            __m128i p2 = _mm_loadu_si128((const __m128i*)&src[x + 8]);
            __m128i p3 = _mm_loadu_si128((const __m128i*)&src[x + 12]);

            // Reorder bytes using shuffle
            _mm_storeu_si128((__m128i*)&dst[x],  _mm_shuffle_epi8(p0, shuffle_mask));
            _mm_storeu_si128((__m128i*)&dst[x + 4],  _mm_shuffle_epi8(p1, shuffle_mask));
            _mm_storeu_si128((__m128i*)&dst[x + 8],  _mm_shuffle_epi8(p2, shuffle_mask));
            _mm_storeu_si128((__m128i*)&dst[x + 12], _mm_shuffle_epi8(p3, shuffle_mask));
        }

        // Handle remaining pixels
        for (; x < width; ++x) {
            ULONG s = src[x];
            dst[x] = SHUFFLE24(s, BGRA32, ARGB32);
        }

        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}

/*
 * The following 24bit->32bit functions only operate on 4 pixels at a time
 */

#define SSE_ABCtoDABC_MASK \
            0xFF, 0x0B, 0x0A, 0x09, \
            0xFF, 0x08, 0x07, 0x06, \
            0xFF, 0x05, 0x04, 0x03, \
            0xFF, 0x02, 0x01, 0x00

ARCHCONVERTFUNCH(SSE3,RGB24,XRGB32)
{
    CONVERTFUNC_INIT

    const __m128i alpha_mask = _mm_set1_epi32(0xFF000000); // Force A=255
    UBYTE *src = (UBYTE *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG y;

    D(bug("[GFX:SSE] %s()\n", __func__);)

    for (y = 0; y < height; y++) {
        ULONG x = 0;
        for (; x + 4 <= width; x += 4) {
            __m128i p0 = _mm_loadu_si128((const __m128i*)&src[x * 3]);
            __m128i mask = _mm_set_epi8(
                SSE_ABCtoDABC_MASK
            );

            // Shuffle into 32-bit words, and store
            __m128i p1 = _mm_shuffle_epi8(p0, mask);
            _mm_storeu_si128((__m128i*)&dst[x], _mm_or_si128(p1, alpha_mask));
        }

        // Handle remaining pixels
        for (; x < width; ++x) {
            dst[x] = GET24;
        }

        src = (UBYTE *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}

#define SSE_ABCtoDCBA_MASK \
            0xFF, 0x09, 0x0A, 0x0B, \
            0xFF, 0x06, 0x07, 0x08, \
            0xFF, 0x03, 0x04, 0x05, \
            0xFF, 0x00, 0x01, 0x02

ARCHCONVERTFUNCH(SSE3,BGR24,XRGB32)
{
    CONVERTFUNC_INIT

    const __m128i alpha_mask = _mm_set1_epi32(0xFF000000); // Force A=255
    UBYTE *src = (UBYTE *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG y = 0;

    D(bug("[GFX:SSE] %s()\n", __func__);)

    for (; y < height; y++) {
        ULONG x = 0;
        for (; x + 4 <= width; x += 4) {
            __m128i p0 = _mm_loadu_si128((const __m128i*)&src[x * 3]);
            __m128i mask = _mm_set_epi8(
                SSE_ABCtoDCBA_MASK
            );

            // Shuffle into 32-bit words, and store
            __m128i p1 = _mm_shuffle_epi8(p0, mask);
            _mm_storeu_si128((__m128i*)&dst[x], _mm_or_si128(p1, alpha_mask));
        }

        // Handle remaining pixels
        for (; x < width; ++x) {
            ULONG s = GET24_INV;
            dst[x] = s;
        }
        src = (UBYTE *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}
#endif

#endif /* __SSE__ */
