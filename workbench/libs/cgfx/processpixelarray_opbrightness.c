/*
    Copyright © 2013-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/cybergraphics.h>

#include <hidd/graphics.h>
#include <cybergraphx/cybergraphics.h>
#include <exec/types.h>

#include "cybergraphics_intern.h"

//#define DEBUG 1
#include <aros/debug.h>

// function is used for both brighen and darken
void ProcessPixelArrayBrightnessFunc(struct RastPort *opRast, struct Rectangle *opRect, LONG value, struct Library *CyberGfxBase)
{
    D(bug("[Cgfx] %s(%d)\n", __PRETTY_FUNCTION__, value));

    LONG x, y;
    ULONG color;
    LONG alpha, red, green, blue;

    if (GetBitMapAttr(opRast->BitMap, BMA_DEPTH) < 15)
    {
        bug("[Cgfx] %s not possible for bitmap depth < 15\n", __PRETTY_FUNCTION__);
        return;
    }

    for(x = opRect->MinX; x <= opRect->MaxX; x++)
    {
        for (y = opRect->MinY; y <= opRect->MaxY; y++)
        {
            color = ReadRGBPixel(opRast, x, y);

            alpha = (color & 0xff);
            red   = (color & 0xff00) >> 8;
            green = (color & 0xff0000) >> 16;
            blue  = (color & 0xff000000) >> 24;

            D(bug("[Cgfx] %s x %d y %d old: alpha %d red %d green %d blue %d", __PRETTY_FUNCTION__, x, y, alpha, red, green, blue));

            red += value;
            green += value;
            blue += value;

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

            color = alpha | (red << 8) | (green << 16) | (blue << 24);

            WriteRGBPixel(opRast, x, y, color);
        }
    }
}
