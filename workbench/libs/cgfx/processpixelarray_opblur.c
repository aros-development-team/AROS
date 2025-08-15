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
void ProcessPixelArrayBlurFunc(struct RastPort *rp, struct Rectangle *rect, struct Library *CyberGfxBase)
{
#if defined(BLUR_USE_GAUSSIAN)
    const float kernel[3] = { 0.27406862f, 0.45186276f, 0.27406862f };
    const int radius = 1;

    LONG width  = rect->MaxX - rect->MinX + 1;
    LONG height = rect->MaxY - rect->MinY + 1;
    LONG bytesPerRow = width * sizeof(ULONG);

    // Horizontal pass (read+write one scanline at a time)
    ULONG *lineBuf = AllocMem(bytesPerRow, MEMF_ANY);
    ULONG *tempBuf = AllocMem(bytesPerRow, MEMF_ANY);
    if (!lineBuf || !tempBuf) {
        bug("[Cgfx] %s failed to allocate horizontal scanline buffers\n", __func__);
        if (lineBuf) FreeMem(lineBuf, bytesPerRow);
        if (tempBuf) FreeMem(tempBuf, bytesPerRow);
        return;
    }

    for (LONG y = 0; y < height; y++) {
        ReadPixelArray(lineBuf, 0, 0, bytesPerRow, rp,
                       rect->MinX, rect->MinY + y, width, 1, RECTFMT_ARGB);

        for (LONG x = 0; x < width; x++) {
            float a = 0, r = 0, g = 0, b = 0;
            for (int k = -radius; k <= radius; k++) {
                int nx = x + k;
                if (nx < 0) nx = 0;
                if (nx >= width) nx = width - 1;
                ULONG p = lineBuf[nx];
                float w = kernel[k + radius];
                a += ((p >> 24) & 0xFF) * w;
                r += ((p >> 16) & 0xFF) * w;
                g += ((p >> 8)  & 0xFF) * w;
                b += ( p        & 0xFF) * w;
            }
            tempBuf[x] =
                ((ULONG)(a + 0.5f) << 24) |
                ((ULONG)(r + 0.5f) << 16) |
                ((ULONG)(g + 0.5f) << 8)  |
                ((ULONG)(b + 0.5f));
        }

        // Write back horizontally blurred line (acts as input for vertical)
        WritePixelArray(tempBuf, 0, 0, bytesPerRow, rp,
                        rect->MinX, rect->MinY + y, width, 1, RECTFMT_ARGB);
    }

    FreeMem(lineBuf, bytesPerRow);
    FreeMem(tempBuf, bytesPerRow);

    // Vertical pass (stream multiple lines for each output line)
    ULONG *colBuf = AllocMem((radius * 2 + 1) * bytesPerRow, MEMF_ANY);
    if (!colBuf) {
        bug("[Cgfx] %s failed to allocate vertical buffer\n", __func__);
        return;
    }

    for (LONG y = 0; y < height; y++) {
        for (int k = -radius; k <= radius; k++) {
            int ny = y + k;
            if (ny < 0) ny = 0;
            if (ny >= height) ny = height - 1;
            ReadPixelArray(colBuf + (k + radius) * width, 0, 0, bytesPerRow, rp,
                           rect->MinX, rect->MinY + ny, width, 1, RECTFMT_ARGB);
        }

        ULONG *outLine = AllocMem(bytesPerRow, MEMF_ANY);
        if (!outLine) {
            bug("[Cgfx] %s failed to allocate vertical outLine buffer\n", __func__);
            FreeMem(colBuf, (radius * 2 + 1) * bytesPerRow);
            return;
        }

        for (LONG x = 0; x < width; x++) {
            float a = 0, r = 0, g = 0, b = 0;
            for (int k = 0; k < radius * 2 + 1; k++) {
                ULONG p = colBuf[k * width + x];
                float w = kernel[k];
                a += ((p >> 24) & 0xFF) * w;
                r += ((p >> 16) & 0xFF) * w;
                g += ((p >> 8)  & 0xFF) * w;
                b += ( p        & 0xFF) * w;
            }
            outLine[x] =
                ((ULONG)(a + 0.5f) << 24) |
                ((ULONG)(r + 0.5f) << 16) |
                ((ULONG)(g + 0.5f) << 8)  |
                ((ULONG)(b + 0.5f));
        }

        WritePixelArray(outLine, 0, 0, bytesPerRow, rp,
                        rect->MinX, rect->MinY + y, width, 1, RECTFMT_ARGB);
        FreeMem(outLine, bytesPerRow);
    }

    FreeMem(colBuf, (radius * 2 + 1) * bytesPerRow);

#else
    // Simple 3x3 box blur streaming
    const int radius = 1;

    LONG width  = rect->MaxX - rect->MinX + 1;
    LONG height = rect->MaxY - rect->MinY + 1;
    LONG bytesPerRow = width * sizeof(ULONG);

    ULONG *rowBuf[3];
    for (int i = 0; i < 3; i++) {
        rowBuf[i] = AllocMem(bytesPerRow, MEMF_ANY);
        if (!rowBuf[i]) {
            bug("[Cgfx] %s failed to allocate box blur buffers\n", __func__);
            while (i--) FreeMem(rowBuf[i], bytesPerRow);
            return;
        }
    }

    // Prime first rows
    for (int i = 0; i < 3; i++) {
        int ysrc = i - 1;
        if (ysrc < 0) ysrc = 0;
        ReadPixelArray(rowBuf[i], 0, 0, bytesPerRow, rp,
                       rect->MinX, rect->MinY + ysrc, width, 1, RECTFMT_ARGB);
    }

    ULONG *outLine = AllocMem(bytesPerRow, MEMF_ANY);
    if (!outLine) {
        for (int i = 0; i < 3; i++) FreeMem(rowBuf[i], bytesPerRow);
        return;
    }

    for (LONG y = 0; y < height; y++) {
        for (LONG x = 0; x < width; x++) {
            unsigned int r = 0, g = 0, b = 0, a = 0, count = 0;

            for (int ky = 0; ky < 3; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    int nx = x + kx;
                    if (nx < 0 || nx >= width) continue;
                    ULONG p = rowBuf[ky][nx];
                    a += (p >> 24) & 0xFF;
                    r += (p >> 16) & 0xFF;
                    g += (p >> 8)  & 0xFF;
                    b +=  p        & 0xFF;
                    count++;
                }
            }
            outLine[x] =
                ((a / count) << 24) |
                ((r / count) << 16) |
                ((g / count) << 8)  |
                (b / count);
        }

        WritePixelArray(outLine, 0, 0, bytesPerRow, rp,
                        rect->MinX, rect->MinY + y, width, 1, RECTFMT_ARGB);

        // Slide buffer down
        if (y + 1 < height) {
            ULONG *tmp = rowBuf[0];
            rowBuf[0] = rowBuf[1];
            rowBuf[1] = rowBuf[2];
            rowBuf[2] = tmp;

            int ysrc = y + 2;
            if (ysrc >= height) ysrc = height - 1;
            ReadPixelArray(rowBuf[2], 0, 0, bytesPerRow, rp,
                           rect->MinX, rect->MinY + ysrc, width, 1, RECTFMT_ARGB);
        }
    }

    FreeMem(outLine, bytesPerRow);
    for (int i = 0; i < 3; i++) FreeMem(rowBuf[i], bytesPerRow);

#endif
}
