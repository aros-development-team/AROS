/*
    Copyright (C) 2013-2025, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/cybergraphics.h>

#include <hidd/gfx.h>
#include <cybergraphx/cybergraphics.h>
#include <exec/types.h>

#include "cybergraphics_intern.h"

void ProcessPixelArrayAlphaFunc(struct RastPort *opRast, struct Rectangle *opRect, UBYTE alphalevel, struct Library *CyberGfxBase)
{
    D(bug("[Cgfx] %s(alphalevel=%d)\n", __func__, alphalevel));

    LONG x, y;
    ULONG color;
    LONG width, height;
    ULONG *linebuf, *ptr;

    if (GetCyberMapAttr(opRast->BitMap, CYBRMATTR_BPPIX) < 4)
    {
        bug("[Cgfx] %s not possible for selected bitmap\n", __func__);
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
                color = (*ptr) & 0xFFFFFF00;
                color |= alphalevel;
                *ptr = color;
                ptr++;
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
