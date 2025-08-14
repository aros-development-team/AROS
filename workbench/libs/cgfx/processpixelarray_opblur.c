/*
    Copyright (C) 2017-2025, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/cybergraphics.h>

#include <hidd/gfx.h>
#include <cybergraphx/cybergraphics.h>
#include <exec/types.h>

#include "cybergraphics_intern.h"

#define BLUR_USE_GAUSSIAN

static inline void ApplyBlur(const ULONG *src, ULONG *dst, LONG width, LONG height)
{
#if defined(BLUR_USE_GAUSSIAN)
    const float kernel[3] = { 0.27406862f, 0.45186276f, 0.27406862f };
    const int radius = 1;

    ULONG *tmp = AllocMem(width * height * 4, MEMF_ANY);
    if (!tmp) {
        bug("[Cgfx] %s failed to allocate storage\n", __func__);
        return;
    }

    // Horizontal pass
    for (LONG y = 0; y < height; y++)
    {
        for (LONG x = 0; x < width; x++)
        {
            float a = 0, r = 0, g = 0, b = 0;
            for (int k = -radius; k <= radius; k++)
            {
                int nx = x + k;
                if (nx < 0) nx = 0;
                if (nx >= width) nx = width - 1;
                ULONG p = src[y * width + nx];
                float w = kernel[k + radius];
                a += ((p >> 24) & 0xFF) * w;
                r += ((p >> 16) & 0xFF) * w;
                g += ((p >> 8)  & 0xFF) * w;
                b += ( p        & 0xFF) * w;
            }
            tmp[y * width + x] =
                ((ULONG)(a + 0.5f) << 24) |
                ((ULONG)(r + 0.5f) << 16) |
                ((ULONG)(g + 0.5f) << 8)  |
                ((ULONG)(b + 0.5f));
        }
    }

    // Vertical pass
    for (LONG y = 0; y < height; y++)
    {
        for (LONG x = 0; x < width; x++)
        {
            float a = 0, r = 0, g = 0, b = 0;
            for (int k = -radius; k <= radius; k++)
            {
                int ny = y + k;
                if (ny < 0) ny = 0;
                if (ny >= height) ny = height - 1;
                ULONG p = tmp[ny * width + x];
                float w = kernel[k + radius];
                a += ((p >> 24) & 0xFF) * w;
                r += ((p >> 16) & 0xFF) * w;
                g += ((p >> 8)  & 0xFF) * w;
                b += ( p        & 0xFF) * w;
            }
            dst[y * width + x] =
                ((ULONG)(a + 0.5f) << 24) |
                ((ULONG)(r + 0.5f) << 16) |
                ((ULONG)(g + 0.5f) << 8)  |
                ((ULONG)(b + 0.5f));
        }
    }

    FreeMem(tmp, width * height * 4);
#else
    for (LONG y = 0; y < height; y++)
    {
        for (LONG x = 0; x < width; x++)
        {
            unsigned int r = 0, g = 0, b = 0, a = 0, count = 0;

            for (int ky = -1; ky <= 1; ky++)
            {
                for (int kx = -1; kx <= 1; kx++)
                {
                    int nx = x + kx;
                    int ny = y + ky;
                    if (nx >= 0 && nx < width && ny >= 0 && ny < height)
                    {
                        ULONG p = src[ny * width + nx];
                        a += (p >> 24) & 0xFF;
                        r += (p >> 16) & 0xFF;
                        g += (p >> 8)  & 0xFF;
                        b +=  p        & 0xFF;
                        count++;
                    }
                }
            }
            dst[y * width + x] =
                ((a / count) << 24) |
                ((r / count) << 16) |
                ((g / count) << 8)  |
                (b / count);
        }
    }
#endif
}

void ProcessPixelArrayBlurFunc(struct RastPort *opRast, struct Rectangle *opRect, struct Library *CyberGfxBase)
{
    LONG width  = opRect->MaxX - opRect->MinX + 1;
    LONG height = opRect->MaxY - opRect->MinY + 1;

    ULONG *readbuf  = AllocMem(width * height * 4, MEMF_ANY);
    ULONG *writebuf = AllocMem(width * height * 4, MEMF_ANY);
    if (!readbuf || !writebuf)
    {
        bug("[Cgfx] %s failed to allocate storage\n", __func__);

        if (readbuf)  FreeMem(readbuf,  width * height * 4);
        if (writebuf) FreeMem(writebuf, width * height * 4);
        return;
    }

    ReadPixelArray(readbuf, 0, 0, width * 4, opRast,
                   opRect->MinX, opRect->MinY, width, height, RECTFMT_ARGB);

    ApplyBlur(readbuf, writebuf, width, height);

    WritePixelArray(writebuf, 0, 0, width * 4, opRast,
                    opRect->MinX, opRect->MinY, width, height, RECTFMT_ARGB);

    FreeMem(readbuf,  width * height * 4);
    FreeMem(writebuf, width * height * 4);
}
