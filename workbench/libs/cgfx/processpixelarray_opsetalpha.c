/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/cybergraphics.h>

#include <hidd/gfx.h>
#include <cybergraphx/cybergraphics.h>
#include <exec/types.h>

#include "cybergraphics_intern.h"

/*
 * Sets the alpha of every pixel; RGB is kept. Only bitmaps with 4 bytes per
 * pixel can hold alpha. RECTFMT_ARGB is a byte order: alpha is byte 0.
 */
void ProcessPixelArrayAlphaFunc(struct RastPort *opRast, struct Rectangle *opRect, UBYTE alphalevel, struct Library *CyberGfxBase)
{
    D(bug("[Cgfx] %s(alphalevel=%d)\n", __func__, alphalevel));

    LONG x, y;
    LONG width, height;
    UBYTE *linebuf, *ptr;

    if (GetCyberMapAttr(opRast->BitMap, CYBRMATTR_BPPIX) < 4)
    {
        bug("[Cgfx] %s not possible for selected bitmap\n", __func__);
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

            for (x = 0, ptr = linebuf; x < width; x++, ptr += 4)
                ptr[0] = alphalevel;

            WritePixelArray(linebuf, 0, 0, width * 4,
                            opRast,
                            opRect->MinX, opRect->MinY + y,
                            width, 1,
                            RECTFMT_ARGB);
        }
        FreeMem(linebuf, width * 4);
    }
}
