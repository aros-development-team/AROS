/*
    Copyright (C) 2013-2017, The AROS Development Team. All rights reserved.
*/

#include <proto/cybergraphics.h>

#include <hidd/gfx.h>
#include <cybergraphx/cybergraphics.h>
#include <exec/types.h>

#include "cybergraphics_intern.h"

//#define DEBUG 1
#include <aros/debug.h>

#include <emmintrin.h>

void ProcessPixelArrayBrightnessFunc(struct RastPort *opRast, struct Rectangle *opRect, LONG value, struct Library *CyberGfxBase)
{
    D(bug("[Cgfx] %s(%d)\n", __func__, value));

    LONG width, height;
    ULONG *buffer;

    if (GetBitMapAttr(opRast->BitMap, BMA_DEPTH) < 15) {
        bug("[Cgfx] %s not possible for bitmap depth < 15\n", __func__);
        return;
    }

    width  = opRect->MaxX - opRect->MinX + 1;
    height = opRect->MaxY - opRect->MinY + 1;

    buffer = AllocMem(width * height * 4, MEMF_ANY);
    if (buffer) {
        ReadPixelArray(buffer, 0, 0, width * 4, opRast,
                       opRect->MinX, opRect->MinY, width, height, RECTFMT_ARGB);

        ULONG total_pixels = width * height;

        __m128i *p = (__m128i*)buffer;
        ULONG simd_count = total_pixels / 4; // 4 pixels per 128-bit register

        // Prepare constants
        __m128i mask_rgb = _mm_set1_epi32(0xFFFFFF00); // mask to keep RGB channels
        __m128i mask_alpha = _mm_set1_epi32(0x000000FF);
        __m128i adjust;

        if (value >= 0) {
            // Positive adjustment ? use saturating add
            adjust = _mm_set1_epi8((char)value); // replicate value to all bytes

            for (ULONG i = 0; i < simd_count; i++) {
                __m128i pix = _mm_loadu_si128(&p[i]);

                // Separate alpha and RGB
                __m128i alpha = _mm_and_si128(pix, mask_alpha);
                __m128i rgb   = _mm_and_si128(pix, mask_rgb);

                // Shift RGB down so each component is in its own byte (ARGB layout assumed)
                // Add adjust to all bytes (alpha included, but we restore alpha later)
                rgb = _mm_adds_epu8(rgb, adjust);

                // Restore alpha
                pix = _mm_or_si128(rgb, alpha);

                _mm_storeu_si128(&p[i], pix);
            }
        } else {
            // Negative adjustment ? use saturating subtract
            adjust = _mm_set1_epi8((char)(-value));

            for (ULONG i = 0; i < simd_count; i++) {
                __m128i pix = _mm_loadu_si128(&p[i]);

                __m128i alpha = _mm_and_si128(pix, mask_alpha);
                __m128i rgb   = _mm_and_si128(pix, mask_rgb);

                rgb = _mm_subs_epu8(rgb, adjust);

                pix = _mm_or_si128(rgb, alpha);

                _mm_storeu_si128(&p[i], pix);
            }
        }

        // Handle leftovers
        ULONG r, *p_remain = (ULONG *)(p + simd_count);
        for (r = simd_count * 4; r < total_pixels; r++) {
            ULONG c = *p_remain;

            LONG alpha = c & 0xff;
            LONG red   = (c >> 8) & 0xff;
            LONG green = (c >> 16) & 0xff;
            LONG blue  = (c >> 24) & 0xff;

            red   += value;
            green += value;
            blue  += value;

            if (red > 255)
                red = 255;
            else if (red < 0)
                red = 0;
            if (green > 255)
                green = 255;
            else if (green < 0)
                green = 0;
            if (blue > 255)
                blue = 255;
            else if (blue < 0)
                blue = 0;

            *p_remain = alpha | (red << 8) | (green << 16) | (blue << 24);
            p_remain++;
        }

        WritePixelArray(buffer, 0, 0, width * 4, opRast,
                        opRect->MinX, opRect->MinY, width, height, RECTFMT_ARGB);

        FreeMem(buffer, width * height * 4);
    }
}

