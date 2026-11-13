/*
    Copyright (C) 2017-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/cybergraphics.h>

#include <hidd/gfx.h>
#include <cybergraphx/cybergraphics.h>
#include <exec/types.h>

#include "cybergraphics_intern.h"

#define BLUR_USE_GAUSSIAN

#if defined(BLUR_USE_GAUSSIAN)
/*
 * Binomial approximation of a Gaussian, 2 * radius + 1 taps, normalised.
 * Float, as the x86_64 SIMD versions, so that every implementation gives
 * the same result.
 */
static void build_gaussian_kernel(float *kernel, int radius)
{
    int n = 2 * radius;
    long double coeff = 1.0;
    long double sum = 0.0;

    for (int k = 0; k <= n; k++) {
        if (k == 0) coeff = 1.0;
        else coeff = coeff * (n - k + 1) / k;
        kernel[k] = (float)coeff;
        sum += coeff;
    }
    for (int i = 0; i <= n; i++)
        kernel[i] /= (float)sum;
}

static inline UBYTE blur_round(float v)
{
    if (v <= 0.0f)
        return 0;
    if (v >= 255.0f)
        return 255;
    return (UBYTE)(v + 0.5f);
}

/*
 * One pass over w * h pixels of 4 bytes (all four components blurred alike,
 * so the byte order does not matter): step is 4 for a horizontal pass, the
 * row size for a vertical one; samples beyond an edge repeat the edge pixel.
 */
static void blur_pass(UBYTE *dst, const UBYTE *src, LONG w, LONG h, LONG horizontal,
                      const float *kernel, int radius)
{
    LONG len   = horizontal ? w : h;
    LONG lines = horizontal ? h : w;
    LONG step  = horizontal ? 4 : w * 4;
    LONG lstep = horizontal ? w * 4 : 4;

    for (LONG l = 0; l < lines; l++) {
        const UBYTE *in = src + l * lstep;
        UBYTE *out = dst + l * lstep;

        for (LONG i = 0; i < len; i++) {
            float acc[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

            for (int k = -radius; k <= radius; k++) {
                LONG n = i + k;
                const UBYTE *p;
                float wgt = kernel[k + radius];

                if (n < 0) n = 0;
                if (n >= len) n = len - 1;
                p = in + n * step;
                acc[0] += p[0] * wgt;
                acc[1] += p[1] * wgt;
                acc[2] += p[2] * wgt;
                acc[3] += p[3] * wgt;
            }
            out[i * step + 0] = blur_round(acc[0]);
            out[i * step + 1] = blur_round(acc[1]);
            out[i * step + 2] = blur_round(acc[2]);
            out[i * step + 3] = blur_round(acc[3]);
        }
    }
}
#endif

void ProcessPixelArrayBlurFunc(struct RastPort *rp, struct Rectangle *rect, LONG value, struct Library *CyberGfxBase)
{
#if defined(BLUR_USE_GAUSSIAN)
    // Clamp value to [0..255] and scale to radius [1..32]
    if (value < 0) value = 0;
    if (value > 255) value = 255;
    int radius = 1 + (value * 31) / 255;
    int kernelSize = radius * 2 + 1;

    LONG width  = rect->MaxX - rect->MinX + 1;
    LONG height = rect->MaxY - rect->MinY + 1;
    ULONG size = width * height * 4;
    float *kernel;
    UBYTE *src, *tmp;

    if (width <= 0 || height <= 0)
        return;

    /*
     * The whole rectangle is read once and written once: both passes must
     * see unblurred input, which writing finished lines back into the
     * RastPort between passes (or lines) does not give.
     */
    kernel = AllocMem(sizeof(float) * kernelSize, MEMF_ANY);
    src = AllocMem(size, MEMF_ANY);
    tmp = AllocMem(size, MEMF_ANY);
    if (kernel && src && tmp) {
        build_gaussian_kernel(kernel, radius);

        ReadPixelArray(src, 0, 0, width * 4, rp,
                       rect->MinX, rect->MinY, width, height, RECTFMT_ARGB);
        blur_pass(tmp, src, width, height, TRUE, kernel, radius);
        blur_pass(src, tmp, width, height, FALSE, kernel, radius);
        WritePixelArray(src, 0, 0, width * 4, rp,
                        rect->MinX, rect->MinY, width, height, RECTFMT_ARGB);
    } else
        bug("[Cgfx] %s failed to allocate the blur buffers\n", __func__);

    if (kernel) FreeMem(kernel, sizeof(float) * kernelSize);
    if (src) FreeMem(src, size);
    if (tmp) FreeMem(tmp, size);
#else
    // Simple box blur (value = radius)
    int radius = (value < 1) ? 1 : value;
    int kernelSize = radius * 2 + 1;

    LONG width  = rect->MaxX - rect->MinX + 1;
    LONG height = rect->MaxY - rect->MinY + 1;
    LONG bytesPerRow = width * sizeof(ULONG);

    // Allocate rolling row buffer
    ULONG **rowBuf = AllocMem(sizeof(ULONG*) * kernelSize, MEMF_ANY);
    if (!rowBuf) return;
    for (int i = 0; i < kernelSize; i++) {
        rowBuf[i] = AllocMem(bytesPerRow, MEMF_ANY);
        if (!rowBuf[i]) {
            bug("[Cgfx] %s failed to allocate box blur buffers\n", __func__);
            while (i--) FreeMem(rowBuf[i], bytesPerRow);
            FreeMem(rowBuf, sizeof(ULONG*) * kernelSize);
            return;
        }
    }

    // Prime initial rows
    for (int i = 0; i < kernelSize; i++) {
        int ysrc = i - radius;
        if (ysrc < 0) ysrc = 0;
        ReadPixelArray(rowBuf[i], 0, 0, bytesPerRow, rp,
                       rect->MinX, rect->MinY + ysrc, width, 1, RECTFMT_ARGB);
    }

    ULONG *outLine = AllocMem(bytesPerRow, MEMF_ANY);
    if (!outLine) {
        for (int i = 0; i < kernelSize; i++) FreeMem(rowBuf[i], bytesPerRow);
        FreeMem(rowBuf, sizeof(ULONG*) * kernelSize);
        return;
    }

    for (LONG y = 0; y < height; y++) {
        for (LONG x = 0; x < width; x++) {
            unsigned int r = 0, g = 0, b = 0, a = 0, count = 0;

            for (int ky = 0; ky < kernelSize; ky++) {
                for (int kx = -radius; kx <= radius; kx++) {
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
            for (int i = 0; i < kernelSize - 1; i++)
                rowBuf[i] = rowBuf[i + 1];
            rowBuf[kernelSize - 1] = tmp;

            int ysrc = y + radius + 1;
            if (ysrc >= height) ysrc = height - 1;
            ReadPixelArray(rowBuf[kernelSize - 1], 0, 0, bytesPerRow, rp,
                           rect->MinX, rect->MinY + ysrc, width, 1, RECTFMT_ARGB);
        }
    }

    FreeMem(outLine, bytesPerRow);
    for (int i = 0; i < kernelSize; i++) FreeMem(rowBuf[i], bytesPerRow);
    FreeMem(rowBuf, sizeof(ULONG*) * kernelSize);

#endif
}
