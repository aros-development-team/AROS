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
    ULONG *buffer;

    width  = opRect->MaxX - opRect->MinX + 1;
    height = opRect->MaxY - opRect->MinY + 1;

    buffer = AllocMem(width * height * 4, MEMF_ANY);
    if (!buffer)
        return;

    ReadPixelArray(buffer, 0, 0, width * 4, opRast,
                   opRect->MinX, opRect->MinY, width, height, RECTFMT_ARGB);

    // Extract tint RGB (ignore alpha)
    ULONG tr = (tintval >> 16) & 0xFF;
    ULONG tg = (tintval >>  8) & 0xFF;
    ULONG tb =  tintval        & 0xFF;

    for (LONG i = 0; i < width * height; i++)
    {
        ULONG p = buffer[i];

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

        buffer[i] = (a << 24) | (r << 16) | (g << 8) | b;
    }

    WritePixelArray(buffer, 0, 0, width * 4, opRast,
                    opRect->MinX, opRect->MinY, width, height, RECTFMT_ARGB);

    FreeMem(buffer, width * height * 4);
}