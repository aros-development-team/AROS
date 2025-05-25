/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#if defined(__AVX__)
#define DEBUG 0
#include <aros/debug.h>

#include <exec/types.h>
#include <hidd/gfx.h>

#include <immintrin.h> // For AVX

#include "colorconv/rgbconv_macros.h"

#undef ARCHCONVERTFUNCH
#define ARCHCONVERTFUNCH(arch, a, b) \
ULONG convert_ ## a ## _ ## b ## _ ## arch \
    (APTR srcPixels, ULONG srcMod, HIDDT_StdPixFmt srcPixFmt, \
    APTR dstPixels, ULONG dstMod, HIDDT_StdPixFmt dstPixFmt, \
    UWORD width, UWORD height) \
{

#define PIXELSPERCONV (4 << 2)

ARCHCONVERTFUNCH(AVX,XRGB32,BGRA32)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG y = 0;

    const __m256i mask_r = _mm256_set1_epi32(0x00FF0000);
    const __m256i mask_g = _mm256_set1_epi32(0x0000FF00);
    const __m256i mask_b  = _mm256_set1_epi32(0x000000FF);

    D(bug("[GFX:AVX] %s()\n", __func__);)

    for (; y < height; ++y) {
        ULONG x = 0;
        for (; x + PIXELSPERCONV <= width; x += PIXELSPERCONV) {
            // Load 16 pixels (2 x 256-bit registers)
            __m256i p0 = _mm256_loadu_si256((const __m256i*)&src[x]);
            __m256i p1 = _mm256_loadu_si256((const __m256i*)&src[x + 8]);

            // Process lower 8 pixels
            __m256i r0 = _mm256_srli_epi32(_mm256_and_si256(p0, mask_r), 8);
            __m256i g0 = _mm256_slli_epi32(_mm256_and_si256(p0, mask_g), 8);
            __m256i b0 = _mm256_slli_epi32(_mm256_and_si256(p0, mask_b), 24);
            __m256i bgra0 = _mm256_or_si256(mask_b, _mm256_or_si256(r0, _mm256_or_si256(g0, b0)));

            // Process upper 8 pixels
            __m256i r1 = _mm256_srli_epi32(_mm256_and_si256(p1, mask_r), 8);
            __m256i g1 = _mm256_slli_epi32(_mm256_and_si256(p1, mask_g), 8);
            __m256i b1 = _mm256_slli_epi32(_mm256_and_si256(p1, mask_b), 24);
            __m256i bgra1 = _mm256_or_si256(mask_b, _mm256_or_si256(r1, _mm256_or_si256(g1, b1)));

            // Store results
            _mm256_storeu_si256((__m256i*)&dst[x],  bgra0);
            _mm256_storeu_si256((__m256i*)&dst[x + 8],  bgra1);
        }

        // handle remaining pixels
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

ARCHCONVERTFUNCH(AVX,BGRA32,XRGB32)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG y = 0;

    const __m256i mask_b  = _mm256_set1_epi32(0xFF000000);
    const __m256i mask_g = _mm256_set1_epi32(0x00FF0000);
    const __m256i mask_r = _mm256_set1_epi32(0x0000FF00);

    D(bug("[GFX:AVX] %s()\n", __func__);)

    for (; y < height; ++y) {
        ULONG x = 0;
        for (; x + PIXELSPERCONV <= width; x += PIXELSPERCONV) {
            // Load 16 pixels (2 x 256-bit registers)
            __m256i p0 = _mm256_loadu_si256((const __m256i *)&src[x]);
            __m256i p1 = _mm256_loadu_si256((const __m256i *)&src[x + 8]);

            // Convert lower 8 pixels
            __m256i r0 = _mm256_slli_epi32(_mm256_and_si256(p0, mask_r), 8);
            __m256i g0 = _mm256_srli_epi32(_mm256_and_si256(p0,  mask_g), 8);
            __m256i b0 = _mm256_srli_epi32(_mm256_and_si256(p0, mask_b), 24);
            __m256i xrgb0 = _mm256_or_si256(mask_b, _mm256_or_si256(r0, _mm256_or_si256(g0, b0)));

            // Convert upper 8 pixels
            __m256i r1 = _mm256_slli_epi32(_mm256_and_si256(p1, mask_r), 8);
            __m256i g1 = _mm256_srli_epi32(_mm256_and_si256(p1,  mask_g), 8);
            __m256i b1 = _mm256_srli_epi32(_mm256_and_si256(p1, mask_b), 24);
            __m256i xrgb1 = _mm256_or_si256(mask_b, _mm256_or_si256(r1, _mm256_or_si256(g1, b1)));

            // Store results
            _mm256_storeu_si256((__m256i *)&dst[x], xrgb0);
            _mm256_storeu_si256((__m256i *)&dst[x + 8], xrgb1);
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

#define AVX_ABCtoDABC_MASK \
            0xFF, 0x1B, 0x1A, 0x19, \
            0xFF, 0x18, 0x17, 0x16, \
            0xFF, 0x15, 0x14, 0x13, \
            0xFF, 0x12, 0x11, 0x10, \
            0xFF, 0x0B, 0x0A, 0x09, \
            0xFF, 0x08, 0x07, 0x06, \
            0xFF, 0x05, 0x04, 0x03, \
            0xFF, 0x02, 0x01, 0x01

ARCHCONVERTFUNCH(AVX,RGB24,XRGB32)
{
    CONVERTFUNC_INIT

    const __m256i alpha_mask = _mm256_set1_epi32(0xFF000000);
    UBYTE *src = (UBYTE *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG y;

    D(bug("[GFX:AVX] %s()\n", __func__);)

    for (y = 0; y < height; y++) {
        ULONG x = 0;
        for (; x + 4 <= width; x += 4) {
            __m128i hi = _mm_loadu_si128((__m128i *)&src[x * 3 + 12]);
            __m128i lo = _mm_loadu_si128((__m128i *)&src[x * 3]);
#if !defined(_mm256_set_m128i)
            __m256i p0 = _mm256_insertf128_si256(_mm256_castsi128_si256(lo), hi, 1);
#else
            __m256i p0 = _mm256_set_m128i(hi, lo);  // Combine into 256-bit register
#endif
            __m256i mask = _mm256_set_epi8(
                AVX_ABCtoDABC_MASK
            );

            // Shuffle bytes using the mask
            __m256i p1 = _mm256_shuffle_epi8(p0, mask);

            // Store the result with the alpha channel set
            _mm256_storeu_si256((__m256i*)&dst[x], _mm256_or_si256(p1, alpha_mask));
        }

        // Handle remaining pixels
        for (; x < width; ++x) {
            ULONG s = GET24;
            dst[x] = s;
        }

        src = (UBYTE *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}

#define AVX_ABCtoDCBA_MASK \
            0xFF, 0x19, 0x1A, 0x1B, \
            0xFF, 0x16, 0x17, 0x18, \
            0xFF, 0x13, 0x14, 0x15, \
            0xFF, 0x10, 0x11, 0x12, \
            0xFF, 0x09, 0x0A, 0x0B, \
            0xFF, 0x06, 0x07, 0x08, \
            0xFF, 0x03, 0x04, 0x05, \
            0xFF, 0x00, 0x01, 0x02

ARCHCONVERTFUNCH(AVX,BGR24,XRGB32)
{
    CONVERTFUNC_INIT

    const __m256i alpha_mask = _mm256_set1_epi32(0xFF000000);
    UBYTE *src = (UBYTE *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG y = 0;

    D(bug("[GFX:AVX] %s()\n", __func__);)

    for (; y < height; y++) {
        ULONG x = 0;
        for (; x + 8 <= width; x += 8) {
            __m128i hi = _mm_loadu_si128((__m128i *)&src[x * 3 + 12]);
            __m128i lo = _mm_loadu_si128((__m128i *)&src[x * 3]);
#if !defined(_mm256_set_m128i)
            __m256i p0 = _mm256_insertf128_si256(_mm256_castsi128_si256(lo), hi, 1);
#else
            __m256i p0 = _mm256_set_m128i(hi, lo);  // Combine into 256-bit register
#endif
            __m256i mask = _mm256_set_epi8(
                AVX_ABCtoDCBA_MASK
            );

            // Shuffle bytes using the mask
            __m256i p1 = _mm256_shuffle_epi8(p0, mask);

            // Store the result with the alpha channel set
            _mm256_storeu_si256((__m256i*)&dst[x], _mm256_or_si256(p1, alpha_mask));
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

/*
 * 32bit <->15bit ops
 */

ARCHCONVERTFUNCH(AVX,BGRA32,RGB15)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;
    UWORD *dst = (UWORD *)dstPixels;
    ULONG y = 0;

    D(bug("[GFX:AVX] %s()\n", __func__);)

    for (; y < height; y++) {
        ULONG x = 0;
        for (; x + 8 <= width; x += 8) {
            // Load 8 x 32-bit pixels
            __m256i pixels = _mm256_loadu_si256((__m256i*)&src[x]);

            // Extract R, G, B
            __m256i r = _mm256_srli_epi32(pixels, 8);
            __m256i g = _mm256_srli_epi32(pixels, 16);
            __m256i b = _mm256_srli_epi32(pixels, 24);

            // Mask to 5 MSBs (>>3 = divide by 8)
            r = _mm256_srli_epi32(r, 3);                           // 8-bit R ? 5-bit
            g = _mm256_srli_epi32(g, 3);
            b = _mm256_srli_epi32(b, 3);

            r = _mm256_and_si256(r, _mm256_set1_epi32(0x1F));
            g = _mm256_and_si256(g, _mm256_set1_epi32(0x1F));
            b = _mm256_and_si256(b, _mm256_set1_epi32(0x1F));

            // Shift to their positions in 15-bit RGB
            r = _mm256_slli_epi32(r, 10);  // R in bits 10–14
            g = _mm256_slli_epi32(g, 5);   // G in bits 5–9
            // B is in bits 0–4 (already there)

            // Combine R, G, B
            __m256i rgb15 = _mm256_or_si256(r, _mm256_or_si256(g, b));

            // Pack 32-bit -> 16-bit
            __m128i low = _mm256_castsi256_si128(rgb15);
            __m128i high = _mm256_extracti128_si256(rgb15, 1);

            __m128i packed = _mm_packus_epi32(low, high);  // Now 8 x uint16_t in 128-bit

            // Store 8 packed 15-bit values
            _mm_storeu_si128((__m128i*)&dst[x], packed);
        }

        // Handle remaining pixels
        for (; x < width; ++x) {
            ULONG s = src[x];
            dst[x] = DOWNSHIFT16(s, BGRA32, RGB15);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}

ARCHCONVERTFUNCH(AVX,BGRA32,BGR15)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;
    UWORD *dst = (UWORD *)dstPixels;
    ULONG y = 0;

    D(bug("[GFX:AVX] %s()\n", __func__);)

    for (; y < height; y++) {
        ULONG x = 0;
        for (; x + 8 <= width; x += 8) {
            // Load 8 x 32-bit pixels
            __m256i pixels = _mm256_loadu_si256((__m256i*)&src[x]);

            // Extract R, G, B
            __m256i r = _mm256_srli_epi32(pixels, 8);
            __m256i g = _mm256_srli_epi32(pixels, 16);
            __m256i b = _mm256_srli_epi32(pixels, 24);

            // Mask to 5 MSBs (>>3 = divide by 8)
            r = _mm256_srli_epi32(r, 3);                           // 8-bit R ? 5-bit
            g = _mm256_srli_epi32(g, 3);
            b = _mm256_srli_epi32(b, 3);

            r = _mm256_and_si256(r, _mm256_set1_epi32(0x1F));
            g = _mm256_and_si256(g, _mm256_set1_epi32(0x1F));
            b = _mm256_and_si256(b, _mm256_set1_epi32(0x1F));

            // Shift to their positions in 15-bit RGB
            b = _mm256_slli_epi32(r, 10);  // R in bits 10–14
            g = _mm256_slli_epi32(g, 5);   // G in bits 5–9
            // r is in bits 0–4 (already there)

            // Combine R, G, B
            __m256i rgb15 = _mm256_or_si256(r, _mm256_or_si256(g, b));

            // Pack 32-bit -> 16-bit
            __m128i low = _mm256_castsi256_si128(rgb15);
            __m128i high = _mm256_extracti128_si256(rgb15, 1);

            __m128i packed = _mm_packus_epi32(low, high);  // Now 8 x uint16_t in 128-bit

            // Store 8 packed 15-bit values
            _mm_storeu_si128((__m128i*)&dst[x], packed);
        }

        // Handle remaining pixels
        for (; x < width; ++x) {
            ULONG s = src[x];
            dst[x] = DOWNSHIFT16(s, BGRA32, BGR15);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}

ARCHCONVERTFUNCH(AVX,ARGB32,RGB15)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;
    UWORD *dst = (UWORD *)dstPixels;
    ULONG y = 0;

    D(bug("[GFX:AVX] %s()\n", __func__);)

    for (; y < height; y++) {
        ULONG x = 0;
        for (; x + 8 <= width; x += 8) {
            // Load 8 x 32-bit pixels
            __m256i pixels = _mm256_loadu_si256((__m256i*)&src[x]);

            // Extract R, G, B
            __m256i r = _mm256_srli_epi32(pixels, 16);
            __m256i g = _mm256_srli_epi32(pixels, 8);
            __m256i b = pixels;

            // Mask to 5 MSBs (>>3 = divide by 8)
            r = _mm256_srli_epi32(r, 3);                           // 8-bit R ? 5-bit
            g = _mm256_srli_epi32(g, 3);
            b = _mm256_srli_epi32(b, 3);

            r = _mm256_and_si256(r, _mm256_set1_epi32(0x1F));
            g = _mm256_and_si256(g, _mm256_set1_epi32(0x1F));
            b = _mm256_and_si256(b, _mm256_set1_epi32(0x1F));

            // Shift to their positions in 15-bit RGB
            r = _mm256_slli_epi32(r, 10);  // R in bits 10–14
            g = _mm256_slli_epi32(g, 5);   // G in bits 5–9
            // B is in bits 0–4 (already there)

            // Combine R, G, B
            __m256i rgb15 = _mm256_or_si256(r, _mm256_or_si256(g, b));

            // Pack 32-bit -> 16-bit
            __m128i low = _mm256_castsi256_si128(rgb15);
            __m128i high = _mm256_extracti128_si256(rgb15, 1);

            __m128i packed = _mm_packus_epi32(low, high);  // Now 8 x uint16_t in 128-bit

            // Store 8 packed 15-bit values
            _mm_storeu_si128((__m128i*)&dst[x], packed);
        }

        // Handle remaining pixels
        for (; x < width; ++x) {
            ULONG s = src[x];
            dst[x] = DOWNSHIFT16(s, ARGB32, RGB15);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}

ARCHCONVERTFUNCH(AVX,ARGB32,BGR15)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;
    UWORD *dst = (UWORD *)dstPixels;
    ULONG y = 0;

    D(bug("[GFX:AVX] %s()\n", __func__);)

    for (; y < height; y++) {
        ULONG x = 0;
        for (; x + 8 <= width; x += 8) {
            // Load 8 x 32-bit pixels
            __m256i pixels = _mm256_loadu_si256((__m256i*)&src[x]);

            // Extract R, G, B
            __m256i r = _mm256_srli_epi32(pixels, 16);
            __m256i g = _mm256_srli_epi32(pixels, 8);
            __m256i b = pixels;

            // Mask to 5 MSBs (>>3 = divide by 8)
            r = _mm256_srli_epi32(r, 3);                           // 8-bit R ? 5-bit
            g = _mm256_srli_epi32(g, 3);
            b = _mm256_srli_epi32(b, 3);

            r = _mm256_and_si256(r, _mm256_set1_epi32(0x1F));
            g = _mm256_and_si256(g, _mm256_set1_epi32(0x1F));
            b = _mm256_and_si256(b, _mm256_set1_epi32(0x1F));

            // Shift to their positions in 15-bit RGB
            b = _mm256_slli_epi32(r, 10);  // R in bits 10–14
            g = _mm256_slli_epi32(g, 5);   // G in bits 5–9
            // r is in bits 0–4 (already there)

            // Combine R, G, B
            __m256i rgb15 = _mm256_or_si256(r, _mm256_or_si256(g, b));

            // Pack 32-bit -> 16-bit
            __m128i low = _mm256_castsi256_si128(rgb15);
            __m128i high = _mm256_extracti128_si256(rgb15, 1);

            __m128i packed = _mm_packus_epi32(low, high);  // Now 8 x uint16_t in 128-bit

            // Store 8 packed 15-bit values
            _mm_storeu_si128((__m128i*)&dst[x], packed);
        }

        // Handle remaining pixels
        for (; x < width; ++x) {
            ULONG s = src[x];
            dst[x] = DOWNSHIFT16(s, ARGB32, BGR15);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}

ARCHCONVERTFUNCH(AVX,BGR15,ARGB32)
{
    CONVERTFUNC_INIT

    UWORD *src = (UWORD *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG y = 0;

    D(bug("[GFX:AVX] %s()\n", __func__);)

    for (; y < height; y++) {
        ULONG x = 0;
        for (; x + 8 <= width; x += 8) {
            // Load 8 x 16-bit BGR15 pixels
            __m128i bgr15 = _mm_loadu_si128((__m128i*)&src[x]); // 8x16-bit

            // Zero-extend to 32-bit ints
            __m256i pixels = _mm256_cvtepu16_epi32(bgr15);

            // Extract 5-bit R, G, B components
            __m256i blue  = _mm256_and_si256(pixels, _mm256_set1_epi32(0x1F));
            __m256i green = _mm256_and_si256(_mm256_srli_epi32(pixels, 5), _mm256_set1_epi32(0x1F));
            __m256i red   = _mm256_and_si256(_mm256_srli_epi32(pixels, 10), _mm256_set1_epi32(0x1F));

            // Scale 5-bit to 8-bit: (value << 3) | (value >> 2) for better range filling
            blue  = _mm256_or_si256(_mm256_slli_epi32(blue, 3), _mm256_srli_epi32(blue, 2));
            green = _mm256_or_si256(_mm256_slli_epi32(green, 3), _mm256_srli_epi32(green, 2));
            red   = _mm256_or_si256(_mm256_slli_epi32(red, 3), _mm256_srli_epi32(red, 2));

            // Pack into ARGB format
            __m256i alpha = _mm256_set1_epi32(0xFF << 24);
            __m256i r = _mm256_slli_epi32(red, 16);
            __m256i g = _mm256_slli_epi32(green, 8);
            __m256i b = blue;

            __m256i argb = _mm256_or_si256(alpha, _mm256_or_si256(r, _mm256_or_si256(g, b)));

            // Store 8 x 32-bit ARGB values
            _mm256_storeu_si256((__m256i*)&dst[x], argb);
        }

        // Handle leftovers
        for (; x < width; ++x) {
            ULONG s = src[x];

            dst[x] = UPSHIFT16(s, BGR15, ARGB32);
        }

        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}

ARCHCONVERTFUNCH(AVX,RGB15,ARGB32)
{
    CONVERTFUNC_INIT

    UWORD *src = (UWORD *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG y = 0;

    D(bug("[GFX:AVX] %s()\n", __func__);)

    for (; y < height; y++) {
        ULONG x = 0;
        for (; x + 8 <= width; x += 8) {
            // Load 8 x 16-bit RGB15 pixels
            __m128i rgb15 = _mm_loadu_si128((__m128i*)&src[x]); // 8 x 16-bit = 128-bit

            // Extend to 32-bit integers
            __m256i pixels = _mm256_cvtepu16_epi32(rgb15);  // 8 x 32-bit

            // Extract 5-bit R, G, B components
            __m256i red   = _mm256_and_si256(pixels, _mm256_set1_epi32(0x1F));              // bits 0–4
            __m256i green = _mm256_and_si256(_mm256_srli_epi32(pixels, 5), _mm256_set1_epi32(0x1F));  // bits 5–9
            __m256i blue  = _mm256_and_si256(_mm256_srli_epi32(pixels, 10), _mm256_set1_epi32(0x1F)); // bits 10–14

            // Scale from 5-bit to 8-bit: (val << 3) | (val >> 2)
            red   = _mm256_or_si256(_mm256_slli_epi32(red, 3), _mm256_srli_epi32(red, 2));
            green = _mm256_or_si256(_mm256_slli_epi32(green, 3), _mm256_srli_epi32(green, 2));
            blue  = _mm256_or_si256(_mm256_slli_epi32(blue, 3), _mm256_srli_epi32(blue, 2));

            // Pack into ARGB format
            __m256i alpha = _mm256_set1_epi32(0xFF << 24);
            __m256i r = _mm256_slli_epi32(red, 16);
            __m256i g = _mm256_slli_epi32(green, 8);
            __m256i b = blue;

            __m256i argb = _mm256_or_si256(alpha, _mm256_or_si256(r, _mm256_or_si256(g, b)));

            // Store 8 x 32-bit ARGB pixels
            _mm256_storeu_si256((__m256i*)&dst[x], argb);
        }

        // Handle remaining pixels (scalar fallback)
        for (; x < width; ++x) {
            ULONG s = src[x];

            dst[x] = UPSHIFT16(s, RGB15, ARGB32);
        }

        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}

ARCHCONVERTFUNCH(AVX,ARGB32,RGB24)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;   // 32-bit ARGB
    UBYTE *dst = (UBYTE *)dstPixels;   // 24-bit RGB
    ULONG y = 0;

    D(bug("[GFX:AVX] %s()\n", __func__);)

    for (; y < height; y++) {
        ULONG x = 0;

        for (; x + 8 <= width; x += 8) {
            // Load 8 x 32-bit ARGB pixels
            __m256i pixels = _mm256_loadu_si256((__m256i*)&src[x]);

            // Extract lower and upper 128-bit lanes for SSE processing
            __m128i lo = _mm256_castsi256_si128(pixels);               // pixels 0-3
            __m128i hi = _mm256_extracti128_si256(pixels, 1);          // pixels 4-7

            // Shuffle to get packed RGB from ARGB
            const __m128i rgb_shuffle = _mm_set_epi8(
                13,14,15,   // R3, G3, B3
                9, 10, 11,   // R2, G2, B2
                 5, 6, 7,   // R1, G1, B1
                 1, 2, 3,    // R0, G0, B0
                 -1, -1, -1, -1 // Padding bytes -> 0x80 (zero fill)
            );
            __m128i rgb0 = _mm_shuffle_epi8(lo, rgb_shuffle);
            __m128i rgb1 = _mm_shuffle_epi8(hi, rgb_shuffle);

            // Write first 12 bytes from rgb0 (store 16, use 12)
            _mm_storeu_si128((__m128i*)&dst[x * 3], rgb0);

            // Write 12 bytes from rgb1 directly to dst
            _mm_storel_epi64((__m128i*)&dst[x * 3 + 12], rgb1);
            *((ULONG *)&dst[x * 3 + 20]) = _mm_cvtsi128_si32(_mm_srli_si128(rgb1, 8));
        }

        // Handle remaining pixels
        for (; x < width; ++x) {
            ULONG s = src[x];

            PUT24(dst, COMP8(s, 1), COMP8(s, 2), COMP8(s, 3))
        }

        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UBYTE *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}

ARCHCONVERTFUNCH(AVX,BGRA32,RGB24)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;   // 32-bit BGRA
    UBYTE *dst = (UBYTE *)dstPixels;   // 24-bit RGB
    ULONG y = 0;

    D(bug("[GFX:AVX] %s()\n", __func__);)

    for (; y < height; y++) {
        ULONG x = 0;

        for (; x + 8 <= width; x += 8) {
            // Load 8 x 32-bit BGRA pixels
            __m256i pixels = _mm256_loadu_si256((__m256i*)&src[x]);

            // Extract lower and upper 128-bit lanes for SSE processing
            __m128i lo = _mm256_castsi256_si128(pixels);               // pixels 0-3
            __m128i hi = _mm256_extracti128_si256(pixels, 1);          // pixels 4-7

            // Shuffle to get packed RGB from BGRA
            const __m128i rgb_shuffle = _mm_set_epi8(
                14,13,12,   // R3, G3, B3
                10, 9, 8,   // R2, G2, B2
                 6, 5, 4,   // R1, G1, B1
                 2, 1, 0,    // R0, G0, B0
                 -1, -1, -1, -1 // Padding bytes -> 0x80 (zero fill)
            );
            __m128i rgb0 = _mm_shuffle_epi8(lo, rgb_shuffle);
            __m128i rgb1 = _mm_shuffle_epi8(hi, rgb_shuffle);

            // Write first 12 bytes from rgb0 (store 16, use 12)
            _mm_storeu_si128((__m128i*)&dst[x * 3], rgb0);

            // Write 12 bytes from rgb1 directly to dst
            _mm_storel_epi64((__m128i*)&dst[x * 3 + 12], rgb1);
            *((ULONG *)&dst[x * 3 + 20]) = _mm_cvtsi128_si32(_mm_srli_si128(rgb1, 8));
        }

        // Handle remaining pixels
        for (; x < width; ++x) {
            ULONG s = src[x];

            PUT24(dst, COMP8(s, 2), COMP8(s, 1), COMP8(s, 0))
        }

        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UBYTE *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}


#endif /* __AVX__ */
