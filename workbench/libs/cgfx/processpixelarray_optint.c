/*
    Copyright (C) 2013-2025, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <aros/debug.h>

#include <proto/cybergraphics.h>

#include <hidd/gfx.h>
#include <cybergraphx/cybergraphics.h>
#include <exec/types.h>

#include "cybergraphics_intern.h"

#define TINT_MODE_MULTIPLY

void ProcessPixelArrayTintFunc(struct RastPort *opRast, struct Rectangle *opRect, ULONG tintval, struct Library *CyberGfxBase)
{
    LONG width, height;
    ULONG *linebuf, *ptr;
    ULONG tr, tg, tb;

    width  = opRect->MaxX - opRect->MinX + 1;
    height = opRect->MaxY - opRect->MinY + 1;

    linebuf = AllocMem(width * sizeof(ULONG), MEMF_ANY);
    if (linebuf) {
        tr = (tintval >> 16) & 0xFF;
        tg = (tintval >>  8) & 0xFF;
        tb =  tintval        & 0xFF;

        for (LONG y = 0; y < height; y++) {
            ReadPixelArray(linebuf, 0, 0, width * sizeof(ULONG),
                           opRast,
                           opRect->MinX, opRect->MinY + y,
                           width, 1,
                           RECTFMT_ARGB);

            ptr = linebuf;
            for (LONG x = 0; x < width; x++) {
                ULONG p = *ptr;

                ULONG a = (p >> 24) & 0xFF;
                ULONG r = (p >> 16) & 0xFF;
                ULONG g = (p >>  8) & 0xFF;
                ULONG b =  p        & 0xFF;

#if defined(TINT_MODE_MULTIPLY)
                // Multiplicative tint
                r = (r * tr) / 255;
                g = (g * tg) / 255;
                b = (b * tb) / 255;
#else
                // Additive tint (clamp to 255)
                r = r + tr; if (r > 255) r = 255;
                g = g + tg; if (g > 255) g = 255;
                b = b + tb; if (b > 255) b = 255;
#endif

                *ptr++ = (a << 24) | (r << 16) | (g << 8) | b;
            }

            WritePixelArray(linebuf, 0, 0, width * sizeof(ULONG),
                            opRast,
                            opRect->MinX, opRect->MinY + y,
                            width, 1,
                            RECTFMT_ARGB);
        }

        FreeMem(linebuf, width * sizeof(ULONG));
    }
}
