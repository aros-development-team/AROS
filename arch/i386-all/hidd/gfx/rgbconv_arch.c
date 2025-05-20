/*
    Copyright (C) 2013-2017, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>
#include <hidd/gfx.h>

#include "colorconv/rgbconv_macros.h"

#define SCCF(SRCPIXFMT, DSTPIXFMT) \
    rgbconvertfuncs[FMT_##SRCPIXFMT - FIRST_RGB_STDPIXFMT][FMT_##DSTPIXFMT - FIRST_RGB_STDPIXFMT] = convert_##SRCPIXFMT##_##DSTPIXFMT##_SSE;

void SetArchRGBConversionFunctions(HIDDT_RGBConversionFunction rgbconvertfuncs[NUM_RGB_STDPIXFMT][NUM_RGB_STDPIXFMT])
{
    /*
     * Following color conversion routines are most used on 32bit displays
     * (VESA, X11, Nouveau) and should be reimplemented using SSE
     *
     * XRGB32 -> BGRA32
     * BGRA32 -> XRGB32
     * BGR24  -> XRGB32
     * BGRX32 -> XRGB32
     * ARGB32 -> BGRA32
     *
     * (See rgbconv_macros.h for definitions of XRGB32,... etc)
     */
#if (0)
    SCCF(XRGB32,BGRA32) 
    SCCF(BGRA32,XRGB32) 
    SCCF(BGR24,XRGB32) 
    SCCF(BGRX32,XRGB32)
#if (0)
    SCCF(ARGB32,BGRA32)
#else
    rgbconvertfuncs[FMT_ARGB32 - FIRST_RGB_STDPIXFMT][FMT_BGRA32 - FIRST_RGB_STDPIXFMT] = convert_BGRX32_XRGB32_SSE;
#endif
#endif
}
