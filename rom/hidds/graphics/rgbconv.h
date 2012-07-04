/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef RGBCONV_H
#define RGBCONV_H

#ifndef HIDD_GRAPHICS_H
#   include <hidd/graphics.h>
#endif

ULONG Convert_RGB24_To_BGR032(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);
ULONG Convert_BGR24_To_BGR032(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);
ULONG Convert_RGB16_To_BGR032(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);
ULONG Convert_RGB16LE_To_BGR032(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);
ULONG Convert_BGR16_To_BGR032(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);
ULONG Convert_BGR16LE_To_BGR032(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);
ULONG Convert_ARGB32_To_BGR032(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);
ULONG Convert_BGRA32_To_BGR032(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);
ULONG Convert_RGBA32_To_BGR032(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);
ULONG Convert_ABGR32_To_BGR032(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);
ULONG Convert_0RGB32_To_BGR032(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);
ULONG Convert_BGR032_To_BGR032(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);
ULONG Convert_RGB032_To_BGR032(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);
ULONG Convert_0BGR32_To_BGR032(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);

ULONG Convert_BGR032_To_ARGB32(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);

ULONG Convert_RGB24_To_RGB16LE(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);
ULONG Convert_BGR24_To_RGB16LE(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);
ULONG Convert_RGB16_To_RGB16LE(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);
ULONG Convert_RGB16LE_To_RGB16LE(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);
ULONG Convert_BGR16_To_RGB16LE(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);
ULONG Convert_BGR16LE_To_RGB16LE(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);
ULONG Convert_ARGB32_To_RGB16LE(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);
ULONG Convert_BGRA32_To_RGB16LE(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);
ULONG Convert_RGBA32_To_RGB16LE(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);
ULONG Convert_ABGR32_To_RGB16LE(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);
ULONG Convert_0RGB32_To_RGB16LE(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);
ULONG Convert_BGR032_To_RGB16LE(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);
ULONG Convert_RGB032_To_RGB16LE(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);
ULONG Convert_0BGR32_To_RGB16LE(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);

ULONG Convert_RGB16LE_To_ARGB32(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height);

#endif /* RGBCONV_H */
