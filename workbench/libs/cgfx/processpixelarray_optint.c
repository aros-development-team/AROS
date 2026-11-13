/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/cybergraphics.h>

#include <hidd/gfx.h>
#include <cybergraphx/cybergraphics.h>
#include <exec/types.h>

#include "cybergraphics_intern.h"

/* c * t / 255, rounded to nearest, for 0 <= c, t <= 255 */
static inline UBYTE mul255(UBYTE c, UBYTE t)
{
    ULONG v = (ULONG)c * t + 128;

    return (v + (v >> 8)) >> 8;
}

/*
 * Multiplies red, green and blue by the tint colour's (0x00RRGGBB) matching
 * component / 255; alpha is kept. RECTFMT_ARGB is a byte order (A, R, G, B
 * in memory), so the components are addressed as bytes.
 */
void ProcessPixelArrayTintFunc(struct RastPort *opRast, struct Rectangle *opRect, ULONG tintval, struct Library *CyberGfxBase)
{
    LONG width, height;
    UBYTE *linebuf, *ptr;
    UBYTE tr, tg, tb;

    width  = opRect->MaxX - opRect->MinX + 1;
    height = opRect->MaxY - opRect->MinY + 1;

    linebuf = AllocMem(width * 4, MEMF_ANY);
    if (linebuf) {
        tr = (tintval >> 16) & 0xFF;
        tg = (tintval >>  8) & 0xFF;
        tb =  tintval        & 0xFF;

        for (LONG y = 0; y < height; y++) {
            ReadPixelArray(linebuf, 0, 0, width * 4,
                           opRast,
                           opRect->MinX, opRect->MinY + y,
                           width, 1,
                           RECTFMT_ARGB);

            ptr = linebuf;
            for (LONG x = 0; x < width; x++, ptr += 4) {
                ptr[1] = mul255(ptr[1], tr);
                ptr[2] = mul255(ptr[2], tg);
                ptr[3] = mul255(ptr[3], tb);
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
