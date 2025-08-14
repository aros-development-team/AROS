/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/cybergraphics.h>

#include <hidd/gfx.h>
#include <cybergraphx/cybergraphics.h>
#include <exec/types.h>

#include "cybergraphics_intern.h"

#include <emmintrin.h>

void ProcessPixelArrayAlphaFunc(struct RastPort *opRast, struct Rectangle *opRect, UBYTE alphalevel, struct Library *CyberGfxBase)
{
    D(bug("[Cgfx] %s(%d)\n", __func__, alphalevel));

    LONG width, height;
    ULONG *buffer;

    if (GetBitMapAttr(opRast->BitMap, BMA_DEPTH) < 32) {
        bug("[Cgfx] %s not possible for bitmap depth < 32\n", __func__);
        return;
    }

    width  = opRect->MaxX - opRect->MinX + 1;
    height = opRect->MaxY - opRect->MinY + 1;

    buffer = AllocMem(width * height * 4, MEMF_ANY);
    if (buffer) {
        ReadPixelArray(buffer, 0, 0, width * 4, opRast,
                       opRect->MinX, opRect->MinY, width, height, RECTFMT_ARGB);

        // Prepare SSE2 constants
        __m128i mask_keep_rgb = _mm_set1_epi32(0xFFFFFF00);       // mask to keep RGB
        __m128i alpha_val     = _mm_set1_epi32(alphalevel);       // alpha in low byte

        ULONG total_pixels = width * height;
        ULONG simd_count = total_pixels / 4; // 4 pixels per 128-bit register
        ULONG i = 0;

        __m128i *p = (__m128i*)buffer;

        for (; i < simd_count; i++) {
            __m128i pix = _mm_loadu_si128(&p[i]);              // load 4 pixels
            pix = _mm_and_si128(pix, mask_keep_rgb);           // clear alpha channel
            pix = _mm_or_si128(pix, alpha_val);                // set new alpha
            _mm_storeu_si128(&p[i], pix);                      // store back
        }

        ULONG r, *p_remain = (ULONG *)(p + i);
        for (r = simd_count * 4; r < total_pixels; r++) {
            *p_remain = (*p_remain & 0xFFFFFF00) | alphalevel;
            p_remain++;
        }

        WritePixelArray(buffer, 0, 0, width * 4, opRast,
                        opRect->MinX, opRect->MinY, width, height, RECTFMT_ARGB);

        FreeMem(buffer, width * height * 4);
    }
}

