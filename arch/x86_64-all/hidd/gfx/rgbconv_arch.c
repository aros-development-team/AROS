/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/types.h>
#include <hidd/gfx.h>

#include "colorconv/rgbconv_macros.h"

#undef ARCHCONVERTFUNCP
#define ARCHCONVERTFUNCP(arch, a, b) \
extern ULONG convert_ ## a ## _ ## b ## _ ## arch \
    (APTR srcPixels, ULONG srcMod, HIDDT_StdPixFmt srcPixFmt, \
    APTR dstPixels, ULONG dstMod, HIDDT_StdPixFmt dstPixFmt, \
    UWORD width, UWORD height);

#define SCCFSSE2(SRCPIXFMT, DSTPIXFMT) \
    rgbconvertfuncs[FMT_##SRCPIXFMT - FIRST_RGB_STDPIXFMT][FMT_##DSTPIXFMT - FIRST_RGB_STDPIXFMT] = convert_##SRCPIXFMT##_##DSTPIXFMT##_SSE2;

#define SCCFSSE3(SRCPIXFMT, DSTPIXFMT) \
    rgbconvertfuncs[FMT_##SRCPIXFMT - FIRST_RGB_STDPIXFMT][FMT_##DSTPIXFMT - FIRST_RGB_STDPIXFMT] = convert_##SRCPIXFMT##_##DSTPIXFMT##_SSE3;

#define SCCFAVX(SRCPIXFMT, DSTPIXFMT) \
    rgbconvertfuncs[FMT_##SRCPIXFMT - FIRST_RGB_STDPIXFMT][FMT_##DSTPIXFMT - FIRST_RGB_STDPIXFMT] = convert_##SRCPIXFMT##_##DSTPIXFMT##_AVX;

/*
 * NB: no ISA preprocessor guards here. This dispatcher is deliberately
 * compiled with only baseline ISA enabled, so that the compiler cannot
 * emit extended instructions into always-executed code, and the SSE/AVX
 * implementation modules are always linked on x86_64. The runtime cpuid
 * checks below gate which implementations are actually installed.
 */
ARCHCONVERTFUNCP(SSE2,BGRA32,XRGB32)
ARCHCONVERTFUNCP(SSE2,XRGB32,BGRA32)
ARCHCONVERTFUNCP(SSE3,BGRA32,XRGB32)
ARCHCONVERTFUNCP(SSE3,XRGB32,BGRA32)
ARCHCONVERTFUNCP(SSE3,RGB24,XRGB32)
ARCHCONVERTFUNCP(SSE3,BGR24,XRGB32)
ARCHCONVERTFUNCP(AVX,XRGB32,BGRA32)
ARCHCONVERTFUNCP(AVX,BGRA32,XRGB32)

ARCHCONVERTFUNCP(AVX,RGB24,XRGB32)
ARCHCONVERTFUNCP(AVX,BGR24,XRGB32)
ARCHCONVERTFUNCP(AVX,ARGB32,RGB24)
ARCHCONVERTFUNCP(AVX,BGRA32,RGB24)

ARCHCONVERTFUNCP(AVX,BGRA32,RGB15)
ARCHCONVERTFUNCP(AVX,BGRA32,BGR15)
ARCHCONVERTFUNCP(AVX,ARGB32,RGB15)
ARCHCONVERTFUNCP(AVX,ARGB32,BGR15)

ARCHCONVERTFUNCP(AVX,BGR15,ARGB32)
ARCHCONVERTFUNCP(AVX,RGB15,ARGB32)

#define cpuid(num, subnum) \
    do { asm volatile("cpuid":"=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx):"a"(num),"c"(subnum)); } while(0)

static int has_ssse3() {
    ULONG eax, ebx, ecx, edx;
    cpuid(0x00000001, 0x0);
    return (ecx & (1 << 9)) != 0; // Bit 9 of ECX = SSSE3
}

static int has_avx2() {
    ULONG eax, ebx, ecx, edx;
    int _ret = 0;
    cpuid(0x00000001, 0x0);
    _ret = (ecx & (1 << 28)) != 0; // Bit 28 of ECX = AVX
    if (_ret)
    {
        cpuid(0x00000007, 0x0); // subleaf ECX = 0
        _ret = (ebx & (1 << 5)) != 0; // Bit 5 of EBX = AVX2
    }
    return _ret;
}

void SetArchRGBConversionFunctions(HIDDT_RGBConversionFunction rgbconvertfuncs[NUM_RGB_STDPIXFMT][NUM_RGB_STDPIXFMT])
{
    BOOL useSSE3 = FALSE, useAVX = FALSE;

    D(bug("[GFX:x86_64] %s()\n", __func__);)

    /*
     * Following color conversion routines are most used on 32bit displays
     * (VESA, X11, Nouveau)
     *
     * XRGB32 -> BGRA32
     * BGRA32 -> XRGB32
     * BGR24  -> XRGB32
     * BGRX32 -> XRGB32
     * ARGB32 -> BGRA32
     *
     * (See rgbconv_macros.h for definitions of XRGB32,... etc)
     */

    if (has_ssse3())
        useSSE3 = TRUE;

    if (has_avx2())
        useAVX = TRUE;

    SCCFSSE2(XRGB32,BGRA32)
    SCCFSSE2(BGRA32,XRGB32)
    if (useSSE3)
    {
        D(bug("[GFX:x86_64] %s: using SSSE3 based operations\n", __func__);)
        SCCFSSE3(XRGB32,BGRA32)
        SCCFSSE3(BGRA32,XRGB32)
        SCCFSSE3(BGR24,XRGB32)
        SCCFSSE3(RGB24,XRGB32)
    }
    if (useAVX)
    {
        D(bug("[GFX:x86_64] %s: using AVX based operations\n", __func__);)
        SCCFAVX(XRGB32,BGRA32)
        SCCFAVX(BGRA32,XRGB32)

        SCCFAVX(BGR24,XRGB32)
        SCCFAVX(RGB24,XRGB32)
        SCCFAVX(ARGB32,RGB24)
        SCCFAVX(BGRA32,RGB24)

        SCCFAVX(BGRA32,RGB15)
        SCCFAVX(BGRA32,BGR15)
        SCCFAVX(ARGB32,RGB15)
        SCCFAVX(ARGB32,BGR15)
        SCCFAVX(BGR15,ARGB32)
        SCCFAVX(RGB15,ARGB32)
    }

    rgbconvertfuncs[FMT_XRGB32 - FIRST_RGB_STDPIXFMT][FMT_BGRX32 - FIRST_RGB_STDPIXFMT] = rgbconvertfuncs[FMT_XRGB32 - FIRST_RGB_STDPIXFMT][FMT_BGRA32 - FIRST_RGB_STDPIXFMT];
    rgbconvertfuncs[FMT_BGRX32 - FIRST_RGB_STDPIXFMT][FMT_XRGB32 - FIRST_RGB_STDPIXFMT] = rgbconvertfuncs[FMT_BGRA32 - FIRST_RGB_STDPIXFMT][FMT_XRGB32 - FIRST_RGB_STDPIXFMT];
    rgbconvertfuncs[FMT_ARGB32 - FIRST_RGB_STDPIXFMT][FMT_BGRA32 - FIRST_RGB_STDPIXFMT] = rgbconvertfuncs[FMT_XRGB32 - FIRST_RGB_STDPIXFMT][FMT_BGRA32 - FIRST_RGB_STDPIXFMT];
    rgbconvertfuncs[FMT_BGRA32 - FIRST_RGB_STDPIXFMT][FMT_ARGB32 - FIRST_RGB_STDPIXFMT] = rgbconvertfuncs[FMT_BGRX32 - FIRST_RGB_STDPIXFMT][FMT_XRGB32 - FIRST_RGB_STDPIXFMT];

#if defined(__SSSE3__) || defined(__AVX__)
    if ((useSSE3) || (useAVX))
    {
        rgbconvertfuncs[FMT_RGB24 - FIRST_RGB_STDPIXFMT][FMT_ARGB32 - FIRST_RGB_STDPIXFMT] = rgbconvertfuncs[FMT_RGB24 - FIRST_RGB_STDPIXFMT][FMT_XRGB32 - FIRST_RGB_STDPIXFMT];
        rgbconvertfuncs[FMT_BGR24 - FIRST_RGB_STDPIXFMT][FMT_ARGB32 - FIRST_RGB_STDPIXFMT] = rgbconvertfuncs[FMT_BGR24 - FIRST_RGB_STDPIXFMT][FMT_XRGB32 - FIRST_RGB_STDPIXFMT];
    }
#endif
#if defined(__AVX__)
    if (useAVX)
    {
        rgbconvertfuncs[FMT_XRGB32 - FIRST_RGB_STDPIXFMT][FMT_RGB24 - FIRST_RGB_STDPIXFMT] = rgbconvertfuncs[FMT_ARGB32- FIRST_RGB_STDPIXFMT][FMT_RGB24 - FIRST_RGB_STDPIXFMT];
        rgbconvertfuncs[FMT_BGRX32 - FIRST_RGB_STDPIXFMT][FMT_RGB24 - FIRST_RGB_STDPIXFMT] = rgbconvertfuncs[FMT_BGRA32- FIRST_RGB_STDPIXFMT][FMT_RGB24 - FIRST_RGB_STDPIXFMT];

        rgbconvertfuncs[FMT_BGRX32 - FIRST_RGB_STDPIXFMT][FMT_RGB15 - FIRST_RGB_STDPIXFMT] = rgbconvertfuncs[FMT_BGRA32- FIRST_RGB_STDPIXFMT][FMT_RGB15 - FIRST_RGB_STDPIXFMT];
        rgbconvertfuncs[FMT_BGRX32 - FIRST_RGB_STDPIXFMT][FMT_BGR15 - FIRST_RGB_STDPIXFMT] = rgbconvertfuncs[FMT_BGRA32 - FIRST_RGB_STDPIXFMT][FMT_BGR15- FIRST_RGB_STDPIXFMT];
        rgbconvertfuncs[FMT_XRGB32 - FIRST_RGB_STDPIXFMT][FMT_RGB15 - FIRST_RGB_STDPIXFMT] = rgbconvertfuncs[FMT_ARGB32- FIRST_RGB_STDPIXFMT][FMT_RGB15 - FIRST_RGB_STDPIXFMT];
        rgbconvertfuncs[FMT_XRGB32 - FIRST_RGB_STDPIXFMT][FMT_BGR15 - FIRST_RGB_STDPIXFMT] = rgbconvertfuncs[FMT_ARGB32 - FIRST_RGB_STDPIXFMT][FMT_BGR15- FIRST_RGB_STDPIXFMT];
    }
#endif

}
