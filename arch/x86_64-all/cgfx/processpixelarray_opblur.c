/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/cybergraphics.h>

#include <hidd/gfx.h>
#include <cybergraphx/cybergraphics.h>
#include <exec/types.h>

#include "cybergraphics_intern.h"

#ifdef __clang__
_Pragma("clang attribute push(__attribute__((target(\"ssse3\"))), apply_to=function)")
#elif defined(__GNUC__)
_Pragma("GCC push_options")
_Pragma("GCC target(\"ssse3\")")
#endif

#include <tmmintrin.h>
#include <emmintrin.h>

// weights (sum = 256)
#define W0 70
#define W1 116
#define W2 70

#if defined(__SSSE3__)
static inline __m128i mul_weights_3tap_u8_to_u8(__m128i left, __m128i center, __m128i right)
{
    const __m128i zero = _mm_setzero_si128();

    // Expand to 16-bit lanes
    __m128i l_lo = _mm_unpacklo_epi8(left,   zero);
    __m128i l_hi = _mm_unpackhi_epi8(left,   zero);
    __m128i c_lo = _mm_unpacklo_epi8(center, zero);
    __m128i c_hi = _mm_unpackhi_epi8(center, zero);
    __m128i r_lo = _mm_unpacklo_epi8(right,  zero);
    __m128i r_hi = _mm_unpackhi_epi8(right,  zero);

    const __m128i k0 = _mm_set1_epi16(W0);
    const __m128i k1 = _mm_set1_epi16(W1);
    const __m128i k2 = _mm_set1_epi16(W2);

    // Weighted sum in 16-bit
    __m128i s_lo = _mm_add_epi16(_mm_add_epi16(_mm_mullo_epi16(l_lo, k0),
                                               _mm_mullo_epi16(c_lo, k1)),
                                               _mm_mullo_epi16(r_lo, k2));
    __m128i s_hi = _mm_add_epi16(_mm_add_epi16(_mm_mullo_epi16(l_hi, k0),
                                               _mm_mullo_epi16(c_hi, k1)),
                                               _mm_mullo_epi16(r_hi, k2));

    // Normalize: round and >> 8 (since sum(weights)=256)
    const __m128i rnd = _mm_set1_epi16(128);
    s_lo = _mm_add_epi16(s_lo, rnd);
    s_hi = _mm_add_epi16(s_hi, rnd);
    s_lo = _mm_srli_epi16(s_lo, 8);
    s_hi = _mm_srli_epi16(s_hi, 8);

    return _mm_packus_epi16(s_lo, s_hi);
}

// Horizontal 3-tap (radius=1), clamping edges
static void GaussianRow_SSSE3(ULONG *dst, const ULONG *src, int w)
{
    if (w <= 0) return;

    // Handle first pixel scalar (edge clamp)
    {
        ULONG c = src[0], r = (w > 1 ? src[1] : c), l = c;
        ULONG ca = ( ( ((l>>24)&0xFF)*W0 + ((c>>24)&0xFF)*W1 + ((r>>24)&0xFF)*W2 + 128 ) >> 8 );
        ULONG cr = ( ( ((l>>16)&0xFF)*W0 + ((c>>16)&0xFF)*W1 + ((r>>16)&0xFF)*W2 + 128 ) >> 8 );
        ULONG cg = ( ( ((l>> 8)&0xFF)*W0 + ((c>> 8)&0xFF)*W1 + ((r>> 8)&0xFF)*W2 + 128 ) >> 8 );
        ULONG cb = ( ( ((l    )&0xFF)*W0 + ((c    )&0xFF)*W1 + ((r    )&0xFF)*W2 + 128 ) >> 8 );
        dst[0] = (ca<<24)|(cr<<16)|(cg<<8)|cb;
    }

    int x = 0;
    // Process 4 pixels per iteration
    for (x = 0; x + 4 <= w - 1; x += 4)
    {
        // prev | curr | next windows
        __m128i prev = _mm_loadu_si128((const __m128i*)(src + x - 0)); // [p0 p1 p2 p3]
        __m128i curr = prev;                                           // same starting point
        __m128i next = _mm_loadu_si128((const __m128i*)(src + x + 1)); // [p1 p2 p3 p4]

        // left = shift right by 4 bytes (bring p[-1]..p[2]), clamp by duplicating p0 at left edge
        // but since x starts at 0+… we need left as (p[-1],p0,p1,p2): build using prev and curr:
        __m128i left  = _mm_alignr_epi8(curr, prev, 12); // bring bytes shifted by 12 = 3 pixels
        // right = shift left by 4 bytes: (p1,p2,p3,p4)
        __m128i right = next;

        // For x==0 case above we already handled first pixel scalar; starting here x>=1 so left is valid.

        __m128i out = mul_weights_3tap_u8_to_u8(left, curr, right);
        _mm_storeu_si128((__m128i*)(dst + x), out);
    }

    // Scalar tail (including last pixel)
    for (; x < w; ++x)
    {
        ULONG l = src[x > 0 ? x - 1 : 0];
        ULONG c = src[x];
        ULONG r = src[x + 1 < w ? x + 1 : w - 1];

        ULONG ca = ( ( ((l>>24)&0xFF)*W0 + ((c>>24)&0xFF)*W1 + ((r>>24)&0xFF)*W2 + 128 ) >> 8 );
        ULONG cr = ( ( ((l>>16)&0xFF)*W0 + ((c>>16)&0xFF)*W1 + ((r>>16)&0xFF)*W2 + 128 ) >> 8 );
        ULONG cg = ( ( ((l>> 8)&0xFF)*W0 + ((c>> 8)&0xFF)*W1 + ((r>> 8)&0xFF)*W2 + 128 ) >> 8 );
        ULONG cb = ( ( ((l    )&0xFF)*W0 + ((c    )&0xFF)*W1 + ((r    )&0xFF)*W2 + 128 ) >> 8 );
        dst[x] = (ca<<24)|(cr<<16)|(cg<<8)|cb;
    }
}
#endif

void ProcessPixelArrayBlurFunc(struct RastPort *opRast, struct Rectangle *opRect, LONG value, struct Library *CyberGfxBase)
{
    const int w = opRect->MaxX - opRect->MinX + 1;
    const int h = opRect->MaxY - opRect->MinY + 1;

    ULONG *src = (ULONG*)AllocMem((ULONG)w * h * 4, MEMF_ANY);
    ULONG *tmp = (ULONG*)AllocMem((ULONG)w * h * 4, MEMF_ANY);
    ULONG *dst = (ULONG*)AllocMem((ULONG)w * h * 4, MEMF_ANY);
    if (!src || !tmp || !dst) {
        if (src) FreeMem(src,(ULONG)w*h*4);
        if (tmp) FreeMem(tmp,(ULONG)w*h*4);
        if (dst) FreeMem(dst,(ULONG)w*h*4);
        return;
    }

    ReadPixelArray(src, 0, 0, w * 4, opRast, opRect->MinX, opRect->MinY, w, h, RECTFMT_ARGB);

#if defined(__SSSE3__)
    // Horizontal pass
    for (int y = 0; y < h; ++y)
        GaussianRow_SSSE3(tmp + y * w, src + y * w, w);

    // Vertical pass (reuse same 3-tap on columns; do scalar to keep code short & robust)
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            ULONG t = tmp[(y ? y - 1 : 0) * w + x];
            ULONG c = tmp[y * w + x];
            ULONG b = tmp[(y + 1 < h ? y + 1 : h - 1) * w + x];

            ULONG a = ( ( ((t>>24)&0xFF)*W0 + ((c>>24)&0xFF)*W1 + ((b>>24)&0xFF)*W2 + 128 ) >> 8 );
            ULONG r = ( ( ((t>>16)&0xFF)*W0 + ((c>>16)&0xFF)*W1 + ((b>>16)&0xFF)*W2 + 128 ) >> 8 );
            ULONG g = ( ( ((t>> 8)&0xFF)*W0 + ((c>> 8)&0xFF)*W1 + ((b>> 8)&0xFF)*W2 + 128 ) >> 8 );
            ULONG bl= ( ( ((t    )&0xFF)*W0 + ((c    )&0xFF)*W1 + ((b    )&0xFF)*W2 + 128 ) >> 8 );
            dst[y * w + x] = (a<<24)|(r<<16)|(g<<8)|bl;
        }
    }
#else
    // Full scalar fallback (no SSSE3)
    // Horizontal
    for (int y = 0; y < h; ++y)
    {
        const ULONG *s = src + y * w;
        ULONG *d = tmp + y * w;
        for (int x = 0; x < w; ++x)
        {
            int xl = x > 0 ? x - 1 : 0;
            int xr = x + 1 < w ? x + 1 : w - 1;
            ULONG l = s[xl], c = s[x], r = s[xr];

            ULONG a = ( ( ((l>>24)&0xFF)*W0 + ((c>>24)&0xFF)*W1 + ((r>>24)&0xFF)*W2 + 128 ) >> 8 );
            ULONG rr= ( ( ((l>>16)&0xFF)*W0 + ((c>>16)&0xFF)*W1 + ((r>>16)&0xFF)*W2 + 128 ) >> 8 );
            ULONG g = ( ( ((l>> 8)&0xFF)*W0 + ((c>> 8)&0xFF)*W1 + ((r>> 8)&0xFF)*W2 + 128 ) >> 8 );
            ULONG b = ( ( ((l    )&0xFF)*W0 + ((c    )&0xFF)*W1 + ((r    )&0xFF)*W2 + 128 ) >> 8 );
            d[x] = (a<<24)|(rr<<16)|(g<<8)|b;
        }
    }
    // Vertical
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            int yt = y > 0 ? y - 1 : 0;
            int yb = y + 1 < h ? y + 1 : h - 1;
            ULONG t = tmp[yt * w + x];
            ULONG c = tmp[y  * w + x];
            ULONG b = tmp[yb * w + x];

            ULONG a = ( ( ((t>>24)&0xFF)*W0 + ((c>>24)&0xFF)*W1 + ((b>>24)&0xFF)*W2 + 128 ) >> 8 );
            ULONG r = ( ( ((t>>16)&0xFF)*W0 + ((c>>16)&0xFF)*W1 + ((b>>16)&0xFF)*W2 + 128 ) >> 8 );
            ULONG g = ( ( ((t>> 8)&0xFF)*W0 + ((c>> 8)&0xFF)*W1 + ((b>> 8)&0xFF)*W2 + 128 ) >> 8 );
            ULONG bl= ( ( ((t    )&0xFF)*W0 + ((c    )&0xFF)*W1 + ((b    )&0xFF)*W2 + 128 ) >> 8 );
            dst[y * w + x] = (a<<24)|(r<<16)|(g<<8)|bl;
        }
    }
#endif

    WritePixelArray(dst, 0, 0, w * 4, opRast, opRect->MinX, opRect->MinY, w, h, RECTFMT_ARGB);
    FreeMem(src, (ULONG)w*h*4);
    FreeMem(tmp, (ULONG)w*h*4);
    FreeMem(dst, (ULONG)w*h*4);
}

# ifdef __clang__
    _Pragma( "clang attribute pop" )
# elif defined __GNUC__
  _Pragma( "GCC pop_options" )
# endif
