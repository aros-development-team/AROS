/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.
*/

#include <proto/cybergraphics.h>

#include <hidd/gfx.h>
#include <cybergraphx/cybergraphics.h>
#include <exec/types.h>

#include "cybergraphics_intern.h"

//#define DEBUG 1
#include <aros/debug.h>

/*
 * Used for both brighten (value > 0) and darken (value < 0): value is added
 * to red, green and blue, clamped to 0..255; alpha is kept. RECTFMT_ARGB is
 * a byte order (A, R, G, B in memory), so the components are addressed as
 * bytes, the same on every CPU.
 */
static inline UBYTE clamp255(LONG v)
{
    return (v < 0) ? 0 : ((v > 255) ? 255 : v);
}

void ProcessPixelArrayBrightnessFunc(struct RastPort *opRast, struct Rectangle *opRect, LONG value, struct Library *CyberGfxBase)
{
    D(bug("[Cgfx] %s(%d)\n", __func__, value));

    LONG x, y;
    LONG width, height;
    UBYTE *linebuf, *ptr;

    if (GetBitMapAttr(opRast->BitMap, BMA_DEPTH) < 15)
    {
        bug("[Cgfx] %s not possible for bitmap depth < 15\n", __func__);
        return;
    }

    width  = opRect->MaxX - opRect->MinX + 1;
    height = opRect->MaxY - opRect->MinY + 1;

    linebuf = AllocMem(width * 4, MEMF_ANY);
    if (linebuf) {
        for (y = 0; y < height; y++) {
            ReadPixelArray(linebuf, 0, 0, width * 4,
                           opRast,
                           opRect->MinX, opRect->MinY + y,
                           width, 1,
                           RECTFMT_ARGB);

            for (x = 0, ptr = linebuf; x < width; x++, ptr += 4) {
                ptr[1] = clamp255(ptr[1] + value);
                ptr[2] = clamp255(ptr[2] + value);
                ptr[3] = clamp255(ptr[3] + value);
            }

            WritePixelArray(linebuf, 0, 0, width * 4,
                            opRast,
                            opRect->MinX, opRect->MinY + y,
                            width, 1,
                            RECTFMT_ARGB);
        }

        FreeMem(linebuf, width * 4);
    }
}
