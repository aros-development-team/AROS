/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 2025 The AROS Dev Team

     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.

     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.

     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330, Cambridge,
     MA 02139, USA.
*/

#include <config.h>

#if defined(__SSE4_1__)

#include <smmintrin.h>

#include "addroutines.h"

static inline BOOL
UseFastPath(Fixed64 offset, Fixed64 add, LONG firstOffset, BOOL stopAtZero)
{
    if(stopAtZero) {
        return FALSE;
    }

    if(add != 0x100000000ULL) {
        return FALSE;
    }

    if((offset & 0xffffffffULL) != 0) {
        return FALSE;
    }

    if((LONG)(offset >> 32) == firstOffset) {
        return FALSE;
    }

    return TRUE;
}

static inline __m128i
MulShift16(__m128i samples, __m128i scale)
{
    __m128i prod_even = _mm_mul_epi32(samples, scale);
    __m128i prod_odd = _mm_mul_epi32(_mm_srli_si128(samples, 4), scale);

    prod_even = _mm_srai_epi64(prod_even, 16);
    prod_odd = _mm_srai_epi64(prod_odd, 16);

    __m128i even32 = _mm_shuffle_epi32(prod_even, _MM_SHUFFLE(2, 0, 2, 0));
    __m128i odd32 = _mm_shuffle_epi32(prod_odd, _MM_SHUFFLE(2, 0, 2, 0));

    return _mm_unpacklo_epi32(even32, odd32);
}

LONG
AddLongMono_SSE41(ADDARGS)
{
    LONG *src = Src;
    LONG *dst = *Dst;
    Fixed64 offset = *Offset;
    LONG index = (LONG)(offset >> 32) - 1;
    int i = 0;

    if(!UseFastPath(offset, Add, FirstOffsetI, StopAtZero) || Samples <= 0) {
        return AddLongMono(ADDARGS);
    }

    __m128i scale = _mm_set1_epi32(ScaleLeft);

    for(; i + 4 <= Samples; i += 4) {
        __m128i samples = _mm_loadu_si128((const __m128i *)(src + index + i));
        __m128i scaled = MulShift16(samples, scale);
        __m128i dstv = _mm_loadu_si128((const __m128i *)(dst + i));

        dstv = _mm_add_epi32(dstv, scaled);
        _mm_storeu_si128((__m128i *)(dst + i), dstv);
    }

    for(; i < Samples; i++) {
        LONG sample = src[index + i];
        dst[i] += (LONG)(((long long)ScaleLeft * sample) >> 16);
    }

    *StartPointLeft = src[index + Samples] >> 16;
    *Dst = dst + Samples;
    *Offset = offset + (Fixed64)Samples * Add;

    return Samples;
}

LONG
AddLongStereo_SSE41(ADDARGS)
{
    LONG *src = Src;
    LONG *dst = *Dst;
    Fixed64 offset = *Offset;
    LONG index = (LONG)(offset >> 32) - 1;
    int i = 0;

    if(!UseFastPath(offset, Add, FirstOffsetI, StopAtZero) || Samples <= 0) {
        return AddLongStereo(ADDARGS);
    }

    __m128i scaleLeft = _mm_set1_epi32(ScaleLeft);
    __m128i scaleRight = _mm_set1_epi32(ScaleRight);

    for(; i + 4 <= Samples; i += 4) {
        __m128i samples = _mm_loadu_si128((const __m128i *)(src + index + i));
        __m128i left = MulShift16(samples, scaleLeft);
        __m128i right = MulShift16(samples, scaleRight);
        __m128i out0 = _mm_unpacklo_epi32(left, right);
        __m128i out1 = _mm_unpackhi_epi32(left, right);
        __m128i dst0 = _mm_loadu_si128((const __m128i *)(dst + (i * 2)));
        __m128i dst1 = _mm_loadu_si128((const __m128i *)(dst + (i * 2) + 4));

        dst0 = _mm_add_epi32(dst0, out0);
        dst1 = _mm_add_epi32(dst1, out1);
        _mm_storeu_si128((__m128i *)(dst + (i * 2)), dst0);
        _mm_storeu_si128((__m128i *)(dst + (i * 2) + 4), dst1);
    }

    for(; i < Samples; i++) {
        LONG sample = src[index + i];
        dst[i * 2] += (LONG)(((long long)ScaleLeft * sample) >> 16);
        dst[i * 2 + 1] += (LONG)(((long long)ScaleRight * sample) >> 16);
    }

    *StartPointLeft = src[index + Samples] >> 16;
    *Dst = dst + Samples * 2;
    *Offset = offset + (Fixed64)Samples * Add;

    return Samples;
}

LONG
AddLongsMono_SSE41(ADDARGS)
{
    LONG *src = Src;
    LONG *dst = *Dst;
    Fixed64 offset = *Offset;
    LONG index = ((LONG)(offset >> 32) - 1) * 2;
    int i = 0;

    if(!UseFastPath(offset, Add, FirstOffsetI, StopAtZero) || Samples <= 0) {
        return AddLongsMono(ADDARGS);
    }

    __m128i scaleLeft = _mm_set1_epi32(ScaleLeft);
    __m128i scaleRight = _mm_set1_epi32(ScaleRight);

    for(; i + 2 <= Samples; i += 2) {
        __m128i samples = _mm_loadu_si128((const __m128i *)(src + index + (i * 2)));
        __m128i leftSamples = _mm_shuffle_epi32(samples, _MM_SHUFFLE(2, 0, 2, 0));
        __m128i rightSamples = _mm_shuffle_epi32(samples, _MM_SHUFFLE(3, 1, 3, 1));
        __m128i left = MulShift16(leftSamples, scaleLeft);
        __m128i right = MulShift16(rightSamples, scaleRight);
        __m128i sum = _mm_add_epi32(left, right);
        __m128i dstv = _mm_loadl_epi64((const __m128i *)(dst + i));

        dstv = _mm_add_epi32(dstv, sum);
        _mm_storel_epi64((__m128i *)(dst + i), dstv);
    }

    for(; i < Samples; i++) {
        LONG sampleL = src[index + (i * 2)];
        LONG sampleR = src[index + (i * 2) + 1];
        dst[i] += (LONG)(((long long)ScaleLeft * sampleL) >> 16)
                  + (LONG)(((long long)ScaleRight * sampleR) >> 16);
    }

    *StartPointLeft = src[index + Samples * 2] >> 16;
    *StartPointRight = src[index + Samples * 2 + 1] >> 16;
    *Dst = dst + Samples;
    *Offset = offset + (Fixed64)Samples * Add;

    return Samples;
}

LONG
AddLongsStereo_SSE41(ADDARGS)
{
    LONG *src = Src;
    LONG *dst = *Dst;
    Fixed64 offset = *Offset;
    LONG index = ((LONG)(offset >> 32) - 1) * 2;
    int i = 0;

    if(!UseFastPath(offset, Add, FirstOffsetI, StopAtZero) || Samples <= 0) {
        return AddLongsStereo(ADDARGS);
    }

    __m128i scaleLeft = _mm_set1_epi32(ScaleLeft);
    __m128i scaleRight = _mm_set1_epi32(ScaleRight);

    for(; i + 2 <= Samples; i += 2) {
        __m128i samples = _mm_loadu_si128((const __m128i *)(src + index + (i * 2)));
        __m128i leftSamples = _mm_shuffle_epi32(samples, _MM_SHUFFLE(2, 0, 2, 0));
        __m128i rightSamples = _mm_shuffle_epi32(samples, _MM_SHUFFLE(3, 1, 3, 1));
        __m128i left = MulShift16(leftSamples, scaleLeft);
        __m128i right = MulShift16(rightSamples, scaleRight);
        __m128i out = _mm_unpacklo_epi32(left, right);
        __m128i dstv = _mm_loadu_si128((const __m128i *)(dst + (i * 2)));

        dstv = _mm_add_epi32(dstv, out);
        _mm_storeu_si128((__m128i *)(dst + (i * 2)), dstv);
    }

    for(; i < Samples; i++) {
        LONG sampleL = src[index + (i * 2)];
        LONG sampleR = src[index + (i * 2) + 1];
        dst[i * 2] += (LONG)(((long long)ScaleLeft * sampleL) >> 16);
        dst[i * 2 + 1] += (LONG)(((long long)ScaleRight * sampleR) >> 16);
    }

    *StartPointLeft = src[index + Samples * 2] >> 16;
    *StartPointRight = src[index + Samples * 2 + 1] >> 16;
    *Dst = dst + Samples * 2;
    *Offset = offset + (Fixed64)Samples * Add;

    return Samples;
}

#endif
