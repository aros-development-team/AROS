/*
    Copyright (C) 2013-2025, The AROS Development Team. All rights reserved.
*/

#include <proto/cybergraphics.h>

#include <hidd/gfx.h>
#include <cybergraphx/cybergraphics.h>
#include <exec/types.h>

#include "cybergraphics_intern.h"

//#define DEBUG 1
#include <aros/debug.h>

// function is used for both brighen and darken
void ProcessPixelArrayBrightnessFunc(struct RastPort *opRast, struct Rectangle *opRect, LONG value, struct Library *CyberGfxBase)
{
    D(bug("[Cgfx] %s(%d)\n", __func__, value));

    LONG x, y;
    ULONG color;
    LONG alpha, red, green, blue;
    LONG width, height;
    ULONG *linebuf, *ptr;

    if (GetBitMapAttr(opRast->BitMap, BMA_DEPTH) < 15)
    {
        bug("[Cgfx] %s not possible for bitmap depth < 15\n", __func__);
        return;
    }

    width  = opRect->MaxX - opRect->MinX + 1;
    height = opRect->MaxY - opRect->MinY + 1;

    linebuf = AllocMem(width * sizeof(ULONG), MEMF_ANY);
    if (linebuf) {
        for (y = 0; y < height; y++) {
            ReadPixelArray(linebuf, 0, 0, width * sizeof(ULONG),
                           opRast,
                           opRect->MinX, opRect->MinY + y,
                           width, 1,
                           RECTFMT_ARGB);

            ptr = linebuf;
            for (x = 0; x < width; x++) {
                color = *ptr;

                alpha = (color & 0xFF);
                red   = (color >> 8)  & 0xFF;
                green = (color >> 16) & 0xFF;
                blue  = (color >> 24) & 0xFF;

                D(bug("[Cgfx] %s x %d y %d old: alpha %d red %d green %d blue %d",
                      __func__, x, y, alpha, red, green, blue));

                red   += value;
                green += value;
                blue  += value;

                if (red > 255)
                    red = 255;
                else if (red < 0)
                    red = 0;
                if (green > 255)
                    green = 255;
                else if (green < 0)
                    green = 0;
                if (blue > 255)
                    blue = 255;
                else if (blue < 0)
                    blue = 0;

                D(bug(" new: alpha %d red %d green %d blue %d\n", alpha, red, green, blue));

                color = (blue << 24) | (green << 16) | (red << 8) | alpha;

                *ptr++ = color;
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
