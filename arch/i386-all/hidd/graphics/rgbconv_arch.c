/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <hidd/graphics.h>

#include "colorconv/rgbconv_macros.h"

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
}
