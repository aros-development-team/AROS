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
    D(bug("[Cgfx] %s(%d)\n", __func__, value));

    LONG x, y;
    ULONG color;
    LONG alpha;
    LONG width, height;
    ULONG * buffer, *ptr;

    if (GetBitMapAttr(opRast->BitMap, BMA_DEPTH) < 32)
    {
        bug("[Cgfx] %s not possible for bitmap depth < 32\n", __func__);
        return;
    }

    width  = opRect->MaxX - opRect->MinX + 1;
    height = opRect->MaxY - opRect->MinY + 1;

    ptr = buffer = AllocMem(width * height * 4, MEMF_ANY);
    ReadPixelArray(buffer, 0, 0, width * 4, opRast, opRect->MinX, opRect->MinY, width, height, RECTFMT_ARGB);

    for (y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            color = (*ptr) & 0xFFFFFF00;
            color |= alphalevel;
            *ptr = color;
            ptr++;
        }
    }

    WritePixelArray(buffer, 0, 0, width * 4, opRast, opRect->MinX, opRect->MinY, width, height, RECTFMT_ARGB);
    FreeMem(buffer, width * height * 4);
}
