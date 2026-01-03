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

#if defined(__AVX2__)

#include <immintrin.h>

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

static inline __m256i
MulShift16(__m256i samples, __m256i scale)
{
    __m256i prod_even = _mm256_mul_epi32(samples, scale);
    __m256i prod_odd = _mm256_mul_epi32(_mm256_srli_si256(samples, 4), scale);

    prod_even = _mm256_srai_epi64(prod_even, 16);
    prod_odd = _mm256_srai_epi64(prod_odd, 16);

    __m256i even32 = _mm256_shuffle_epi32(prod_even, _MM_SHUFFLE(2, 0, 2, 0));
    __m256i odd32 = _mm256_shuffle_epi32(prod_odd, _MM_SHUFFLE(2, 0, 2, 0));

    return _mm256_unpacklo_epi32(even32, odd32);
}

LONG
AddLongMono_AVX2(ADDARGS)
{
    LONG *src = Src;
    LONG *dst = *Dst;
    Fixed64 offset = *Offset;
    LONG index = (LONG)(offset >> 32) - 1;
    int i = 0;

    if(!UseFastPath(offset, Add, FirstOffsetI, StopAtZero) || Samples <= 0) {
        return AddLongMono(ADDARGS);
    }

    __m256i scale = _mm256_set1_epi32(ScaleLeft);

    for(; i + 8 <= Samples; i += 8) {
        __m256i samples = _mm256_loadu_si256((const __m256i *)(src + index + i));
        __m256i scaled = MulShift16(samples, scale);
        __m256i dstv = _mm256_loadu_si256((const __m256i *)(dst + i));

        dstv = _mm256_add_epi32(dstv, scaled);
        _mm256_storeu_si256((__m256i *)(dst + i), dstv);
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
AddLongStereo_AVX2(ADDARGS)
{
    LONG *src = Src;
    LONG *dst = *Dst;
    Fixed64 offset = *Offset;
    LONG index = (LONG)(offset >> 32) - 1;
    int i = 0;

    if(!UseFastPath(offset, Add, FirstOffsetI, StopAtZero) || Samples <= 0) {
        return AddLongStereo(ADDARGS);
    }

    __m256i scaleLeft = _mm256_set1_epi32(ScaleLeft);
    __m256i scaleRight = _mm256_set1_epi32(ScaleRight);

    for(; i + 8 <= Samples; i += 8) {
        __m256i samples = _mm256_loadu_si256((const __m256i *)(src + index + i));
        __m256i left = MulShift16(samples, scaleLeft);
        __m256i right = MulShift16(samples, scaleRight);
        __m256i out0 = _mm256_unpacklo_epi32(left, right);
        __m256i out1 = _mm256_unpackhi_epi32(left, right);
        __m256i dst0 = _mm256_loadu_si256((const __m256i *)(dst + (i * 2)));
        __m256i dst1 = _mm256_loadu_si256((const __m256i *)(dst + (i * 2) + 8));

        dst0 = _mm256_add_epi32(dst0, out0);
        dst1 = _mm256_add_epi32(dst1, out1);
        _mm256_storeu_si256((__m256i *)(dst + (i * 2)), dst0);
        _mm256_storeu_si256((__m256i *)(dst + (i * 2) + 8), dst1);
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
AddLongsMono_AVX2(ADDARGS)
{
    LONG *src = Src;
    LONG *dst = *Dst;
    Fixed64 offset = *Offset;
    LONG index = ((LONG)(offset >> 32) - 1) * 2;
    int i = 0;

    if(!UseFastPath(offset, Add, FirstOffsetI, StopAtZero) || Samples <= 0) {
        return AddLongsMono(ADDARGS);
    }

    __m256i scaleLeft = _mm256_set1_epi32(ScaleLeft);
    __m256i scaleRight = _mm256_set1_epi32(ScaleRight);

    for(; i + 4 <= Samples; i += 4) {
        __m256i samples = _mm256_loadu_si256((const __m256i *)(src + index + (i * 2)));
        __m256i leftSamples = _mm256_shuffle_epi32(samples, _MM_SHUFFLE(2, 0, 2, 0));
        __m256i rightSamples = _mm256_shuffle_epi32(samples, _MM_SHUFFLE(3, 1, 3, 1));
        __m256i left = MulShift16(leftSamples, scaleLeft);
        __m256i right = MulShift16(rightSamples, scaleRight);
        __m256i sum = _mm256_add_epi32(left, right);
        __m128i sumLo = _mm256_castsi256_si128(sum);
        __m128i sumHi = _mm256_extracti128_si256(sum, 1);
        __m128i dstLo = _mm_loadl_epi64((const __m128i *)(dst + i));
        __m128i dstHi = _mm_loadl_epi64((const __m128i *)(dst + i + 2));

        dstLo = _mm_add_epi32(dstLo, sumLo);
        dstHi = _mm_add_epi32(dstHi, sumHi);
        _mm_storel_epi64((__m128i *)(dst + i), dstLo);
        _mm_storel_epi64((__m128i *)(dst + i + 2), dstHi);
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
AddLongsStereo_AVX2(ADDARGS)
{
    LONG *src = Src;
    LONG *dst = *Dst;
    Fixed64 offset = *Offset;
    LONG index = ((LONG)(offset >> 32) - 1) * 2;
    int i = 0;

    if(!UseFastPath(offset, Add, FirstOffsetI, StopAtZero) || Samples <= 0) {
        return AddLongsStereo(ADDARGS);
    }

    __m256i scaleLeft = _mm256_set1_epi32(ScaleLeft);
    __m256i scaleRight = _mm256_set1_epi32(ScaleRight);

    for(; i + 4 <= Samples; i += 4) {
        __m256i samples = _mm256_loadu_si256((const __m256i *)(src + index + (i * 2)));
        __m256i leftSamples = _mm256_shuffle_epi32(samples, _MM_SHUFFLE(2, 0, 2, 0));
        __m256i rightSamples = _mm256_shuffle_epi32(samples, _MM_SHUFFLE(3, 1, 3, 1));
        __m256i left = MulShift16(leftSamples, scaleLeft);
        __m256i right = MulShift16(rightSamples, scaleRight);
        __m256i out = _mm256_unpacklo_epi32(left, right);
        __m256i dstv = _mm256_loadu_si256((const __m256i *)(dst + (i * 2)));

        dstv = _mm256_add_epi32(dstv, out);
        _mm256_storeu_si256((__m256i *)(dst + (i * 2)), dstv);
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
