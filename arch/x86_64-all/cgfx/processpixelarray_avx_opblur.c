/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/cybergraphics.h>

#include <hidd/gfx.h>
#include <cybergraphx/cybergraphics.h>
#include <exec/types.h>

#include "cybergraphics_intern.h"

#define BLUR_USE_GAUSSIAN

#ifdef __clang__
_Pragma("clang attribute push(__attribute__((target(\"avx2\"))), apply_to=function)")
#elif defined(__GNUC__)
_Pragma("GCC push_options")
_Pragma("GCC target(\"avx2\")")
#endif

#include <immintrin.h>

static void build_gaussian_kernel(float *kernel, int radius)
{
    int kernelSize = radius * 2 + 1;
    int n = 2 * radius;
    long double coeff = 1.0;
    long double sum = 0.0;

    for (int k = 0; k <= n; k++)
    {
        if (k == 0)
            coeff = 1.0;
        else
            coeff = coeff * (n - k + 1) / k;
        kernel[k] = (float)coeff;
        sum += coeff;
    }
    for (int i = 0; i < kernelSize; i++)
        kernel[i] /= (float)sum;
}

static void gaussian_horizontal_avx(ULONG *dst, const ULONG *src, int w, int h, const float *kernel, int radius)
{
    const __m256i mask = _mm256_set1_epi32(0xFF);
    const __m256 half = _mm256_set1_ps(0.5f);

    for (int y = 0; y < h; ++y)
    {
        const ULONG *row = src + y * w;
        ULONG *out = dst + y * w;
        int x = 0;

        for (; x < radius && x < w; ++x)
        {
            float a = 0.0f, r = 0.0f, g = 0.0f, b = 0.0f;
            for (int k = -radius; k <= radius; ++k)
            {
                int nx = x + k;
                if (nx < 0) nx = 0;
                if (nx >= w) nx = w - 1;
                ULONG p = row[nx];
                float wgt = kernel[k + radius];
                a += ((p >> 24) & 0xFF) * wgt;
                r += ((p >> 16) & 0xFF) * wgt;
                g += ((p >> 8) & 0xFF) * wgt;
                b += (p & 0xFF) * wgt;
            }
            a = a < 0.0f ? 0.0f : (a > 255.0f ? 255.0f : a);
            r = r < 0.0f ? 0.0f : (r > 255.0f ? 255.0f : r);
            g = g < 0.0f ? 0.0f : (g > 255.0f ? 255.0f : g);
            b = b < 0.0f ? 0.0f : (b > 255.0f ? 255.0f : b);
            out[x] = ((ULONG)(a + 0.5f) << 24) | ((ULONG)(r + 0.5f) << 16) | ((ULONG)(g + 0.5f) << 8) | (ULONG)(b + 0.5f);
        }

        int vec_end = w - radius;
        for (; x + 8 <= vec_end; x += 8)
        {
            __m256 acc_a = _mm256_setzero_ps();
            __m256 acc_r = _mm256_setzero_ps();
            __m256 acc_g = _mm256_setzero_ps();
            __m256 acc_b = _mm256_setzero_ps();

            for (int k = -radius; k <= radius; ++k)
            {
                const ULONG *p = row + x + k;
                __m256i pix = _mm256_loadu_si256((const __m256i *)p);
                __m256 wgt = _mm256_set1_ps(kernel[k + radius]);

                __m256i a_i = _mm256_and_si256(_mm256_srli_epi32(pix, 24), mask);
                __m256i r_i = _mm256_and_si256(_mm256_srli_epi32(pix, 16), mask);
                __m256i g_i = _mm256_and_si256(_mm256_srli_epi32(pix, 8), mask);
                __m256i b_i = _mm256_and_si256(pix, mask);

                acc_a = _mm256_add_ps(acc_a, _mm256_mul_ps(_mm256_cvtepi32_ps(a_i), wgt));
                acc_r = _mm256_add_ps(acc_r, _mm256_mul_ps(_mm256_cvtepi32_ps(r_i), wgt));
                acc_g = _mm256_add_ps(acc_g, _mm256_mul_ps(_mm256_cvtepi32_ps(g_i), wgt));
                acc_b = _mm256_add_ps(acc_b, _mm256_mul_ps(_mm256_cvtepi32_ps(b_i), wgt));
            }

            __m256i a32 = _mm256_cvtps_epi32(_mm256_add_ps(acc_a, half));
            __m256i r32 = _mm256_cvtps_epi32(_mm256_add_ps(acc_r, half));
            __m256i g32 = _mm256_cvtps_epi32(_mm256_add_ps(acc_g, half));
            __m256i b32 = _mm256_cvtps_epi32(_mm256_add_ps(acc_b, half));

            /* Clamp to 0-255 to prevent channel contamination */
            __m256i zero = _mm256_setzero_si256();
            a32 = _mm256_max_epi32(_mm256_min_epi32(a32, mask), zero);
            r32 = _mm256_max_epi32(_mm256_min_epi32(r32, mask), zero);
            g32 = _mm256_max_epi32(_mm256_min_epi32(g32, mask), zero);
            b32 = _mm256_max_epi32(_mm256_min_epi32(b32, mask), zero);

            __m256i outv = _mm256_or_si256(_mm256_slli_epi32(a32, 24), _mm256_slli_epi32(r32, 16));
            outv = _mm256_or_si256(outv, _mm256_slli_epi32(g32, 8));
            outv = _mm256_or_si256(outv, b32);
            _mm256_storeu_si256((__m256i *)(out + x), outv);
        }

        for (; x < w; ++x)
        {
            float a = 0.0f, r = 0.0f, g = 0.0f, b = 0.0f;
            for (int k = -radius; k <= radius; ++k)
            {
                int nx = x + k;
                if (nx < 0) nx = 0;
                if (nx >= w) nx = w - 1;
                ULONG p = row[nx];
                float wgt = kernel[k + radius];
                a += ((p >> 24) & 0xFF) * wgt;
                r += ((p >> 16) & 0xFF) * wgt;
                g += ((p >> 8) & 0xFF) * wgt;
                b += (p & 0xFF) * wgt;
            }
            a = a < 0.0f ? 0.0f : (a > 255.0f ? 255.0f : a);
            r = r < 0.0f ? 0.0f : (r > 255.0f ? 255.0f : r);
            g = g < 0.0f ? 0.0f : (g > 255.0f ? 255.0f : g);
            b = b < 0.0f ? 0.0f : (b > 255.0f ? 255.0f : b);
            out[x] = ((ULONG)(a + 0.5f) << 24) | ((ULONG)(r + 0.5f) << 16) | ((ULONG)(g + 0.5f) << 8) | (ULONG)(b + 0.5f);
        }
    }
}

static void gaussian_vertical_avx(ULONG *dst, const ULONG *src, int w, int h, const float *kernel, int radius)
{
    const __m256i mask = _mm256_set1_epi32(0xFF);
    const __m256 half = _mm256_set1_ps(0.5f);

    for (int y = 0; y < h; ++y)
    {
        if (y < radius || y >= h - radius)
        {
            for (int x = 0; x < w; ++x)
            {
                float a = 0.0f, r = 0.0f, g = 0.0f, b = 0.0f;
                for (int k = -radius; k <= radius; ++k)
                {
                    int ny = y + k;
                    if (ny < 0) ny = 0;
                    if (ny >= h) ny = h - 1;
                    ULONG p = src[ny * w + x];
                    float wgt = kernel[k + radius];
                    a += ((p >> 24) & 0xFF) * wgt;
                    r += ((p >> 16) & 0xFF) * wgt;
                    g += ((p >> 8) & 0xFF) * wgt;
                    b += (p & 0xFF) * wgt;
                }
                a = a < 0.0f ? 0.0f : (a > 255.0f ? 255.0f : a);
                r = r < 0.0f ? 0.0f : (r > 255.0f ? 255.0f : r);
                g = g < 0.0f ? 0.0f : (g > 255.0f ? 255.0f : g);
                b = b < 0.0f ? 0.0f : (b > 255.0f ? 255.0f : b);
                dst[y * w + x] = ((ULONG)(a + 0.5f) << 24) | ((ULONG)(r + 0.5f) << 16) | ((ULONG)(g + 0.5f) << 8) | (ULONG)(b + 0.5f);
            }
            continue;
        }

        int x = 0;
        for (; x + 8 <= w; x += 8)
        {
            __m256 acc_a = _mm256_setzero_ps();
            __m256 acc_r = _mm256_setzero_ps();
            __m256 acc_g = _mm256_setzero_ps();
            __m256 acc_b = _mm256_setzero_ps();

            for (int k = -radius; k <= radius; ++k)
            {
                const ULONG *p = src + (y + k) * w + x;
                __m256i pix = _mm256_loadu_si256((const __m256i *)p);
                __m256 wgt = _mm256_set1_ps(kernel[k + radius]);

                __m256i a_i = _mm256_and_si256(_mm256_srli_epi32(pix, 24), mask);
                __m256i r_i = _mm256_and_si256(_mm256_srli_epi32(pix, 16), mask);
                __m256i g_i = _mm256_and_si256(_mm256_srli_epi32(pix, 8), mask);
                __m256i b_i = _mm256_and_si256(pix, mask);

                acc_a = _mm256_add_ps(acc_a, _mm256_mul_ps(_mm256_cvtepi32_ps(a_i), wgt));
                acc_r = _mm256_add_ps(acc_r, _mm256_mul_ps(_mm256_cvtepi32_ps(r_i), wgt));
                acc_g = _mm256_add_ps(acc_g, _mm256_mul_ps(_mm256_cvtepi32_ps(g_i), wgt));
                acc_b = _mm256_add_ps(acc_b, _mm256_mul_ps(_mm256_cvtepi32_ps(b_i), wgt));
            }

            __m256i a32 = _mm256_cvtps_epi32(_mm256_add_ps(acc_a, half));
            __m256i r32 = _mm256_cvtps_epi32(_mm256_add_ps(acc_r, half));
            __m256i g32 = _mm256_cvtps_epi32(_mm256_add_ps(acc_g, half));
            __m256i b32 = _mm256_cvtps_epi32(_mm256_add_ps(acc_b, half));

            /* Clamp to 0-255 to prevent channel contamination */
            __m256i zero = _mm256_setzero_si256();
            a32 = _mm256_max_epi32(_mm256_min_epi32(a32, mask), zero);
            r32 = _mm256_max_epi32(_mm256_min_epi32(r32, mask), zero);
            g32 = _mm256_max_epi32(_mm256_min_epi32(g32, mask), zero);
            b32 = _mm256_max_epi32(_mm256_min_epi32(b32, mask), zero);

            __m256i outv = _mm256_or_si256(_mm256_slli_epi32(a32, 24), _mm256_slli_epi32(r32, 16));
            outv = _mm256_or_si256(outv, _mm256_slli_epi32(g32, 8));
            outv = _mm256_or_si256(outv, b32);
            _mm256_storeu_si256((__m256i *)(dst + y * w + x), outv);
        }

        for (; x < w; ++x)
        {
            float a = 0.0f, r = 0.0f, g = 0.0f, b = 0.0f;
            for (int k = -radius; k <= radius; ++k)
            {
                int ny = y + k;
                ULONG p = src[ny * w + x];
                float wgt = kernel[k + radius];
                a += ((p >> 24) & 0xFF) * wgt;
                r += ((p >> 16) & 0xFF) * wgt;
                g += ((p >> 8) & 0xFF) * wgt;
                b += (p & 0xFF) * wgt;
            }
            a = a < 0.0f ? 0.0f : (a > 255.0f ? 255.0f : a);
            r = r < 0.0f ? 0.0f : (r > 255.0f ? 255.0f : r);
            g = g < 0.0f ? 0.0f : (g > 255.0f ? 255.0f : g);
            b = b < 0.0f ? 0.0f : (b > 255.0f ? 255.0f : b);
            dst[y * w + x] = ((ULONG)(a + 0.5f) << 24) | ((ULONG)(r + 0.5f) << 16) | ((ULONG)(g + 0.5f) << 8) | (ULONG)(b + 0.5f);
        }
    }
}

static void box_blur_avx(ULONG *dst, const ULONG *src, int w, int h, int radius)
{
    const __m256i mask = _mm256_set1_epi32(0xFF);
    const __m256 half = _mm256_set1_ps(0.5f);

    ULONG *sumA = (ULONG *)AllocMem(sizeof(ULONG) * w * h, MEMF_ANY);
    ULONG *sumR = (ULONG *)AllocMem(sizeof(ULONG) * w * h, MEMF_ANY);
    ULONG *sumG = (ULONG *)AllocMem(sizeof(ULONG) * w * h, MEMF_ANY);
    ULONG *sumB = (ULONG *)AllocMem(sizeof(ULONG) * w * h, MEMF_ANY);
    float *countX = (float *)AllocMem(sizeof(float) * w, MEMF_ANY);
    float *countY = (float *)AllocMem(sizeof(float) * h, MEMF_ANY);

    if (!sumA || !sumR || !sumG || !sumB || !countX || !countY)
    {
        if (sumA) FreeMem(sumA, sizeof(ULONG) * w * h);
        if (sumR) FreeMem(sumR, sizeof(ULONG) * w * h);
        if (sumG) FreeMem(sumG, sizeof(ULONG) * w * h);
        if (sumB) FreeMem(sumB, sizeof(ULONG) * w * h);
        if (countX) FreeMem(countX, sizeof(float) * w);
        if (countY) FreeMem(countY, sizeof(float) * h);
        return;
    }

    for (int x = 0; x < w; ++x)
    {
        int left = x - radius;
        int right = x + radius;
        if (left < 0) left = 0;
        if (right >= w) right = w - 1;
        countX[x] = (float)(right - left + 1);
    }
    for (int y = 0; y < h; ++y)
    {
        int top = y - radius;
        int bottom = y + radius;
        if (top < 0) top = 0;
        if (bottom >= h) bottom = h - 1;
        countY[y] = (float)(bottom - top + 1);
    }

    for (int y = 0; y < h; ++y)
    {
        const ULONG *row = src + y * w;
        ULONG *outA = sumA + y * w;
        ULONG *outR = sumR + y * w;
        ULONG *outG = sumG + y * w;
        ULONG *outB = sumB + y * w;
        int x = 0;

        for (; x < radius && x < w; ++x)
        {
            ULONG a = 0, r = 0, g = 0, b = 0;
            for (int k = -radius; k <= radius; ++k)
            {
                int nx = x + k;
                if (nx < 0 || nx >= w)
                    continue;
                ULONG p = row[nx];
                a += (p >> 24) & 0xFF;
                r += (p >> 16) & 0xFF;
                g += (p >> 8) & 0xFF;
                b += p & 0xFF;
            }
            outA[x] = a;
            outR[x] = r;
            outG[x] = g;
            outB[x] = b;
        }

        int vec_end = w - radius;
        for (; x + 8 <= vec_end; x += 8)
        {
            __m256i acc_a = _mm256_setzero_si256();
            __m256i acc_r = _mm256_setzero_si256();
            __m256i acc_g = _mm256_setzero_si256();
            __m256i acc_b = _mm256_setzero_si256();

            for (int k = -radius; k <= radius; ++k)
            {
                const ULONG *p = row + x + k;
                __m256i pix = _mm256_loadu_si256((const __m256i *)p);
                __m256i a_i = _mm256_and_si256(_mm256_srli_epi32(pix, 24), mask);
                __m256i r_i = _mm256_and_si256(_mm256_srli_epi32(pix, 16), mask);
                __m256i g_i = _mm256_and_si256(_mm256_srli_epi32(pix, 8), mask);
                __m256i b_i = _mm256_and_si256(pix, mask);

                acc_a = _mm256_add_epi32(acc_a, a_i);
                acc_r = _mm256_add_epi32(acc_r, r_i);
                acc_g = _mm256_add_epi32(acc_g, g_i);
                acc_b = _mm256_add_epi32(acc_b, b_i);
            }

            _mm256_storeu_si256((__m256i *)(outA + x), acc_a);
            _mm256_storeu_si256((__m256i *)(outR + x), acc_r);
            _mm256_storeu_si256((__m256i *)(outG + x), acc_g);
            _mm256_storeu_si256((__m256i *)(outB + x), acc_b);
        }

        for (; x < w; ++x)
        {
            ULONG a = 0, r = 0, g = 0, b = 0;
            for (int k = -radius; k <= radius; ++k)
            {
                int nx = x + k;
                if (nx < 0 || nx >= w)
                    continue;
                ULONG p = row[nx];
                a += (p >> 24) & 0xFF;
                r += (p >> 16) & 0xFF;
                g += (p >> 8) & 0xFF;
                b += p & 0xFF;
            }
            outA[x] = a;
            outR[x] = r;
            outG[x] = g;
            outB[x] = b;
        }
    }

    for (int y = 0; y < h; ++y)
    {
        float count_y = countY[y];
        int x = 0;

        if (y < radius || y >= h - radius)
        {
            for (x = 0; x < w; ++x)
            {
                ULONG a = 0, r = 0, g = 0, b = 0;
                for (int k = -radius; k <= radius; ++k)
                {
                    int ny = y + k;
                    if (ny < 0 || ny >= h)
                        continue;
                    int idx = ny * w + x;
                    a += sumA[idx];
                    r += sumR[idx];
                    g += sumG[idx];
                    b += sumB[idx];
                }
                float count = countX[x] * count_y;
                dst[y * w + x] = ((ULONG)(a / count + 0.5f) << 24) |
                                 ((ULONG)(r / count + 0.5f) << 16) |
                                 ((ULONG)(g / count + 0.5f) << 8) |
                                 (ULONG)(b / count + 0.5f);
            }
            continue;
        }

        for (; x + 8 <= w; x += 8)
        {
            __m256i acc_a = _mm256_setzero_si256();
            __m256i acc_r = _mm256_setzero_si256();
            __m256i acc_g = _mm256_setzero_si256();
            __m256i acc_b = _mm256_setzero_si256();

            for (int k = -radius; k <= radius; ++k)
            {
                int idx = (y + k) * w + x;
                acc_a = _mm256_add_epi32(acc_a, _mm256_loadu_si256((const __m256i *)(sumA + idx)));
                acc_r = _mm256_add_epi32(acc_r, _mm256_loadu_si256((const __m256i *)(sumR + idx)));
                acc_g = _mm256_add_epi32(acc_g, _mm256_loadu_si256((const __m256i *)(sumG + idx)));
                acc_b = _mm256_add_epi32(acc_b, _mm256_loadu_si256((const __m256i *)(sumB + idx)));
            }

            __m256 countx = _mm256_loadu_ps(countX + x);
            __m256 count = _mm256_mul_ps(countx, _mm256_set1_ps(count_y));

            __m256 a = _mm256_div_ps(_mm256_cvtepi32_ps(acc_a), count);
            __m256 r = _mm256_div_ps(_mm256_cvtepi32_ps(acc_r), count);
            __m256 g = _mm256_div_ps(_mm256_cvtepi32_ps(acc_g), count);
            __m256 b = _mm256_div_ps(_mm256_cvtepi32_ps(acc_b), count);

            __m256i a32 = _mm256_cvtps_epi32(_mm256_add_ps(a, half));
            __m256i r32 = _mm256_cvtps_epi32(_mm256_add_ps(r, half));
            __m256i g32 = _mm256_cvtps_epi32(_mm256_add_ps(g, half));
            __m256i b32 = _mm256_cvtps_epi32(_mm256_add_ps(b, half));

            /* Clamp values to 0-255 to prevent corruption */
            __m256i zero = _mm256_setzero_si256();
            a32 = _mm256_max_epi32(_mm256_min_epi32(a32, mask), zero);
            r32 = _mm256_max_epi32(_mm256_min_epi32(r32, mask), zero);
            g32 = _mm256_max_epi32(_mm256_min_epi32(g32, mask), zero);
            b32 = _mm256_max_epi32(_mm256_min_epi32(b32, mask), zero);

            __m256i outv = _mm256_or_si256(_mm256_slli_epi32(a32, 24), _mm256_slli_epi32(r32, 16));
            outv = _mm256_or_si256(outv, _mm256_slli_epi32(g32, 8));
            outv = _mm256_or_si256(outv, b32);
            _mm256_storeu_si256((__m256i *)(dst + y * w + x), outv);
        }

        for (; x < w; ++x)
        {
            ULONG a = 0, r = 0, g = 0, b = 0;
            for (int k = -radius; k <= radius; ++k)
            {
                int idx = (y + k) * w + x;
                a += sumA[idx];
                r += sumR[idx];
                g += sumG[idx];
                b += sumB[idx];
            }
            float count = countX[x] * count_y;
            int ia = (int)(a / count + 0.5f);
            int ir = (int)(r / count + 0.5f);
            int ig = (int)(g / count + 0.5f);
            int ib = (int)(b / count + 0.5f);
            if (ia < 0) ia = 0; else if (ia > 255) ia = 255;
            if (ir < 0) ir = 0; else if (ir > 255) ir = 255;
            if (ig < 0) ig = 0; else if (ig > 255) ig = 255;
            if (ib < 0) ib = 0; else if (ib > 255) ib = 255;
            dst[y * w + x] = (ia << 24) | (ir << 16) | (ig << 8) | ib;
        }
    }

    FreeMem(sumA, sizeof(ULONG) * w * h);
    FreeMem(sumR, sizeof(ULONG) * w * h);
    FreeMem(sumG, sizeof(ULONG) * w * h);
    FreeMem(sumB, sizeof(ULONG) * w * h);
    FreeMem(countX, sizeof(float) * w);
    FreeMem(countY, sizeof(float) * h);
}

void ProcessPixelArrayBlurFunc_AVX(struct RastPort *opRast, struct Rectangle *opRect, LONG value, struct Library *CyberGfxBase)
{
    const int w = opRect->MaxX - opRect->MinX + 1;
    const int h = opRect->MaxY - opRect->MinY + 1;

    if (w <= 0 || h <= 0)
        return;

    ULONG *src = (ULONG *)AllocMem((ULONG)w * h * 4, MEMF_ANY);
    ULONG *tmp = (ULONG *)AllocMem((ULONG)w * h * 4, MEMF_ANY);
    ULONG *dst = (ULONG *)AllocMem((ULONG)w * h * 4, MEMF_ANY);
    if (!src || !tmp || !dst)
    {
        if (src) FreeMem(src, (ULONG)w * h * 4);
        if (tmp) FreeMem(tmp, (ULONG)w * h * 4);
        if (dst) FreeMem(dst, (ULONG)w * h * 4);
        return;
    }

    ReadPixelArray(src, 0, 0, w * 4, opRast, opRect->MinX, opRect->MinY, w, h, RECTFMT_ARGB);

#if defined(BLUR_USE_GAUSSIAN)
    if (value < 0) value = 0;
    if (value > 255) value = 255;
    int radius = 1 + (value * 31) / 255;
    int kernelSize = radius * 2 + 1;

    float *kernel = (float *)AllocMem(sizeof(float) * kernelSize, MEMF_ANY);
    if (!kernel)
    {
        FreeMem(src, (ULONG)w * h * 4);
        FreeMem(tmp, (ULONG)w * h * 4);
        FreeMem(dst, (ULONG)w * h * 4);
        return;
    }

    build_gaussian_kernel(kernel, radius);
    gaussian_horizontal_avx(tmp, src, w, h, kernel, radius);
    gaussian_vertical_avx(dst, tmp, w, h, kernel, radius);

    FreeMem(kernel, sizeof(float) * kernelSize);
#else
    int radius = value < 1 ? 1 : value;
    box_blur_avx(dst, src, w, h, radius);
#endif

    WritePixelArray(dst, 0, 0, w * 4, opRast, opRect->MinX, opRect->MinY, w, h, RECTFMT_ARGB);
    FreeMem(src, (ULONG)w * h * 4);
    FreeMem(tmp, (ULONG)w * h * 4);
    FreeMem(dst, (ULONG)w * h * 4);
}

# ifdef __clang__
    _Pragma( "clang attribute pop" )
# elif defined __GNUC__
  _Pragma( "GCC pop_options" )
# endif
