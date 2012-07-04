/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: RGB pixel format conversion functions.
*/

#include "rgbconv.h"


ULONG Convert_RGB24_To_BGR032(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height)
{
    WORD x, y, src_step, dst_step;
    UBYTE *p = srcPixels, *q = dstPixels;
    UBYTE src_red, src_green, src_blue;

    src_step = srcMod - width * 3;
    dst_step = dstMod - width * 4;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            src_red   = *p++;
            src_green = *p++;
            src_blue  = *p++;

            *q++ = src_blue;
            *q++ = src_green;
            *q++ = src_red;
            *q++ = 0;
        }
        p += src_step;
        q += dst_step;
    }

    return TRUE;
}


ULONG Convert_BGR24_To_BGR032(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height)
{
    WORD x, y, src_step, dst_step;
    UBYTE *p = srcPixels, *q = dstPixels;
    UBYTE src_red, src_green, src_blue;

    src_step = srcMod - width * 3;
    dst_step = dstMod - width * 4;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            src_blue  = *p++;
            src_green = *p++;
            src_red   = *p++;

            *q++ = src_blue;
            *q++ = src_green;
            *q++ = src_red;
            *q++ = 0;
        }
        p += src_step;
        q += dst_step;
    }

    return TRUE;
}


ULONG Convert_RGB16_To_BGR032(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height)
{
    WORD x, y, src_step, dst_step;
    UBYTE *p = srcPixels, *q = dstPixels;
    UBYTE src_red, src_green, src_blue;

    src_step = srcMod - width * 2;
    dst_step = dstMod - width * 4;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            src_red = *p++;
            src_blue = *p++;
            src_green = src_red << 5 | src_blue >> 3 & 0x1c;
            src_blue <<= 3;
            src_red &= 0xf8;

            *q++ = src_blue;
            *q++ = src_green;
            *q++ = src_red;
            *q++ = 0;
        }
        p += src_step;
        q += dst_step;
    }

    return TRUE;
}


ULONG Convert_RGB16LE_To_BGR032(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height)
{
    WORD x, y, src_step, dst_step;
    UBYTE *p = srcPixels, *q = dstPixels;
    UBYTE src_red, src_green, src_blue;

    src_step = srcMod - width * 2;
    dst_step = dstMod - width * 4;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            src_blue = *p++;
            src_red = *p++;
            src_green = src_red << 5 | src_blue >> 3 & 0x1c;
            src_blue <<= 3;
            src_red &= 0xf8;

            *q++ = src_blue;
            *q++ = src_green;
            *q++ = src_red;
            *q++ = 0;
        }
        p += src_step;
        q += dst_step;
    }

    return TRUE;
}


ULONG Convert_BGR16_To_BGR032(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height)
{
    WORD x, y, src_step, dst_step;
    UBYTE *p = srcPixels, *q = dstPixels;
    UBYTE src_red, src_green, src_blue;

    src_step = srcMod - width * 2;
    dst_step = dstMod - width * 4;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            src_blue = *p++;
            src_red = *p++;
            src_green = src_blue << 5 | src_red >> 3 & 0x1c;
            src_blue &= 0xf8;
            src_red <<= 3;

            *q++ = src_blue;
            *q++ = src_green;
            *q++ = src_red;
            *q++ = 0;
        }
        p += src_step;
        q += dst_step;
    }

    return TRUE;
}


ULONG Convert_BGR16LE_To_BGR032(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height)
{
    WORD x, y, src_step, dst_step;
    UBYTE *p = srcPixels, *q = dstPixels;
    UBYTE src_red, src_green, src_blue;

    src_step = srcMod - width * 2;
    dst_step = dstMod - width * 4;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            src_red = *p++;
            src_blue = *p++;
            src_green = src_blue << 5 | src_red >> 3 & 0x1c;
            src_red <<= 3;
            src_blue &= 0xf8;

            *q++ = src_blue;
            *q++ = src_green;
            *q++ = src_red;
            *q++ = 0;
        }
        p += src_step;
        q += dst_step;
    }

    return TRUE;
}


ULONG Convert_ARGB32_To_BGR032(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height)
{
    WORD x, y, src_step, dst_step;
    UBYTE *p = srcPixels, *q = dstPixels;
    UBYTE src_red, src_green, src_blue;

    src_step = srcMod - width * 4;
    dst_step = dstMod - width * 4;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            p++; /* alpha */
            src_red   = *p++;
            src_green = *p++;
            src_blue  = *p++;

            *q++ = src_blue;
            *q++ = src_green;
            *q++ = src_red;
            *q++ = 0;
        }
        p += src_step;
        q += dst_step;
    }

    return TRUE;
}


ULONG Convert_BGRA32_To_BGR032(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height)
{
    WORD x, y, src_step, dst_step;
    UBYTE *p = srcPixels, *q = dstPixels;
    UBYTE src_red, src_green, src_blue;

    src_step = srcMod - width * 4;
    dst_step = dstMod - width * 4;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            src_blue  = *p++;
            src_green = *p++;
            src_red   = *p++;
            p++; /* alpha */

            *q++ = src_blue;
            *q++ = src_green;
            *q++ = src_red;
            *q++ = 0;
        }
        p += src_step;
        q += dst_step;
    }

    return TRUE;
}


ULONG Convert_RGBA32_To_BGR032(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height)
{
    WORD x, y, src_step, dst_step;
    UBYTE *p = srcPixels, *q = dstPixels;
    UBYTE src_red, src_green, src_blue;

    src_step = srcMod - width * 4;
    dst_step = dstMod - width * 4;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            src_red   = *p++;
            src_green = *p++;
            src_blue  = *p++;
            p++; /* alpha */

            *q++ = src_blue;
            *q++ = src_green;
            *q++ = src_red;
            *q++ = 0;
        }
        p += src_step;
        q += dst_step;
    }

    return TRUE;
}


ULONG Convert_ABGR32_To_BGR032(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height)
{
    WORD x, y, src_step, dst_step;
    UBYTE *p = srcPixels, *q = dstPixels;
    UBYTE src_red, src_green, src_blue;

    src_step = srcMod - width * 4;
    dst_step = dstMod - width * 4;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            p++; /* alpha */
            src_blue  = *p++;
            src_green = *p++;
            src_red   = *p++;

            *q++ = src_blue;
            *q++ = src_green;
            *q++ = src_red;
            *q++ = 0;
        }
        p += src_step;
        q += dst_step;
    }

    return TRUE;
}


ULONG Convert_BGR032_To_ARGB32(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height)
{
    WORD x, y, src_step, dst_step;
    UBYTE *p = srcPixels, *q = dstPixels;
    UBYTE src_red, src_green, src_blue;

    src_step = srcMod - width * 4;
    dst_step = dstMod - width * 4;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            src_blue  = *p++;
            src_green = *p++;
            src_red   = *p++;
            p++; /* zero */

            *q++ = 0xff;
            *q++ = src_red;
            *q++ = src_green;
            *q++ = src_blue;
        }
        p += src_step;
        q += dst_step;
    }

    return TRUE;
}


ULONG Convert_RGB24_To_RGB16LE(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height)
{
    WORD x, y, src_step, dst_step;
    UBYTE *p = srcPixels, *q = dstPixels;
    UBYTE src_red, src_green, src_blue;

    src_step = srcMod - width * 3;
    dst_step = dstMod - width * 2;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            src_red   = *p++;
            src_green = *p++;
            src_blue  = *p++;

            *q++ = (src_green << 3) & 0xe0 | src_blue >> 3;
            *q++ = src_red & 0xf8 | src_green >> 5;
        }
        p += src_step;
        q += dst_step;
    }

    return TRUE;
}


ULONG Convert_BGR24_To_RGB16LE(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height)
{
    WORD x, y, src_step, dst_step;
    UBYTE *p = srcPixels, *q = dstPixels;
    UBYTE src_red, src_green, src_blue;

    src_step = srcMod - width * 3;
    dst_step = dstMod - width * 2;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            src_blue  = *p++;
            src_green = *p++;
            src_red   = *p++;

            *q++ = (src_green << 3) & 0xe0 | src_blue >> 3;
            *q++ = src_red & 0xf8 | src_green >> 5;
        }
        p += src_step;
        q += dst_step;
    }

    return TRUE;
}


ULONG Convert_RGB16_To_RGB16LE(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height)
{
    WORD x, y, src_step, dst_step;
    UBYTE *p = srcPixels, *q = dstPixels;
    UBYTE src_red, src_blue;

    src_step = srcMod - width * 2;
    dst_step = dstMod - width * 2;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            src_red   = *p++;
            src_blue  = *p++;

            *q++ = src_blue;
            *q++ = src_red;
        }
        p += src_step;
        q += dst_step;
    }

    return TRUE;
}


ULONG Convert_RGB16LE_To_RGB16LE(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height)
{
    WORD x, y, src_step, dst_step;
    UBYTE *p = srcPixels, *q = dstPixels;
    UBYTE src_red, src_blue;

    src_step = srcMod - width * 2;
    dst_step = dstMod - width * 2;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            src_blue  = *p++;
            src_red   = *p++;

            *q++ = src_blue;
            *q++ = src_red;
        }
        p += src_step;
        q += dst_step;
    }

    return TRUE;
}


ULONG Convert_BGR16_To_RGB16LE(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height)
{
    WORD x, y, src_step, dst_step;
    UBYTE *p = srcPixels, *q = dstPixels;
    UBYTE src_red, src_green, src_blue;

    src_step = srcMod - width * 4;
    dst_step = dstMod - width * 2;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            src_blue = *p++;
            src_red = *p++;
            src_green = src_blue << 5 | src_red >> 3 & 0x1c;
            src_blue &= 0xf8;
            src_red <<= 3;

            *q++ = (src_green << 3) & 0xe0 | src_blue >> 3;
            *q++ = src_red & 0xf8 | src_green >> 5;
        }
        p += src_step;
        q += dst_step;
    }

    return TRUE;
}


ULONG Convert_BGR16LE_To_RGB16LE(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height)
{
    WORD x, y, src_step, dst_step;
    UBYTE *p = srcPixels, *q = dstPixels;
    UBYTE src_red, src_green, src_blue;

    src_step = srcMod - width * 4;
    dst_step = dstMod - width * 2;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            src_red = *p++;
            src_blue = *p++;
            src_green = src_blue << 5 | src_red >> 3 & 0x1c;
            src_red <<= 3;
            src_blue &= 0xf8;

            *q++ = (src_green << 3) & 0xe0 | src_blue >> 3;
            *q++ = src_red & 0xf8 | src_green >> 5;
        }
        p += src_step;
        q += dst_step;
    }

    return TRUE;
}


ULONG Convert_ARGB32_To_RGB16LE(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height)
{
    WORD x, y, src_step, dst_step;
    UBYTE *p = srcPixels, *q = dstPixels;
    UBYTE src_red, src_green, src_blue;

    src_step = srcMod - width * 4;
    dst_step = dstMod - width * 2;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            p++; /* alpha */
            src_red   = *p++;
            src_green = *p++;
            src_blue  = *p++;

            *q++ = (src_green << 3) & 0xe0 | src_blue >> 3;
            *q++ = src_red & 0xf8 | src_green >> 5;
        }
        p += src_step;
        q += dst_step;
    }

    return TRUE;
}


ULONG Convert_BGRA32_To_RGB16LE(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height)
{
    WORD x, y, src_step, dst_step;
    UBYTE *p = srcPixels, *q = dstPixels;
    UBYTE src_red, src_green, src_blue;

    src_step = srcMod - width * 4;
    dst_step = dstMod - width * 2;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            src_blue  = *p++;
            src_green = *p++;
            src_red   = *p++;
            p++; /* alpha */

            *q++ = (src_green << 3) & 0xe0 | src_blue >> 3;
            *q++ = src_red & 0xf8 | src_green >> 5;
        }
        p += src_step;
        q += dst_step;
    }

    return TRUE;
}


ULONG Convert_RGBA32_To_RGB16LE(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height)
{
    WORD x, y, src_step, dst_step;
    UBYTE *p = srcPixels, *q = dstPixels;
    UBYTE src_red, src_green, src_blue;

    src_step = srcMod - width * 4;
    dst_step = dstMod - width * 2;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            src_red   = *p++;
            src_green = *p++;
            src_blue  = *p++;
            p++; /* alpha */

            *q++ = (src_green << 3) & 0xe0 | src_blue >> 3;
            *q++ = src_red & 0xf8 | src_green >> 5;
        }
        p += src_step;
        q += dst_step;
    }

    return TRUE;
}


ULONG Convert_ABGR32_To_RGB16LE(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height)
{
    WORD x, y, src_step, dst_step;
    UBYTE *p = srcPixels, *q = dstPixels;
    UBYTE src_red, src_green, src_blue;

    src_step = srcMod - width * 4;
    dst_step = dstMod - width * 2;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            p++; /* alpha */
            src_blue  = *p++;
            src_green = *p++;
            src_red   = *p++;

            *q++ = (src_green << 3) & 0xe0 | src_blue >> 3;
            *q++ = src_red & 0xf8 | src_green >> 5;
        }
        p += src_step;
        q += dst_step;
    }

    return TRUE;
}


ULONG Convert_RGB16LE_To_ARGB32(APTR srcPixels, ULONG srcMod,
    HIDDT_StdPixFmt srcPixFmt, APTR dstPixels, ULONG dstMod,
    HIDDT_StdPixFmt dstPixFmt, UWORD width, UWORD height)
{
    WORD x, y, src_step, dst_step;
    UBYTE *p = srcPixels, *q = dstPixels;
    UBYTE src_red, src_green, src_blue;

    src_step = srcMod - width * 2;
    dst_step = dstMod - width * 4;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            src_blue = *p++;
            src_red = *p++;
            src_green = src_red << 5 | src_blue >> 3 & 0x1c;
            src_blue <<= 3;
            src_red &= 0xf8;

            *q++ = 0xff;
            *q++ = src_red;
            *q++ = src_green;
            *q++ = src_blue;
        }
        p += src_step;
        q += dst_step;
    }

    return TRUE;
}


