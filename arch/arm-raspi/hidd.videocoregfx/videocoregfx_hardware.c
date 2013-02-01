/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
 
    Desc: VideoCore4 hardware functions
    Lang: English
*/

#define DEBUG 1
#include <aros/debug.h>

#include <asm/io.h>

#include "videocoregfx_class.h"
#include "videocoregfx_hardware.h"

BOOL FNAME_HW(InitGfxHW)(APTR param0)
{
    struct VideoCore_staticdata *xsd = param0;

    D(bug("[VideoCoreGfx] %s()\n", __PRETTY_FUNCTION__));

    return TRUE;
}

void FNAME_HW(RefreshArea)(struct HWData *hwdata, struct BitmapData *data,
		       LONG x1, LONG y1, LONG x2, LONG y2)
{
/*    struct VideoCore_staticdata *xsd = param0;

    UBYTE *src, *dst;
    ULONG srcmod, dstmod;
    LONG  y, w, h;
    LONG sx, sy;

    x1 += data->xoffset;
    y1 += data->yoffset;
    x2 += data->xoffset;
    y2 += data->yoffset;

    // Clip the rectangle against physical display borders
    if ((x1 >= data->disp_width) || (x2 < 1) ||
        (y1 >= data->disp_height) || (y2 < 1))
	return;
    if (x1 < 0)
	x1 = 0;
    if (y1 < 0)
	y1 = 0;
    if (x2 > data->disp_width)
	x2 = data->disp_width;
    if (y2 > data->disp_height)
	y2 = data->disp_height;

    // Calculate width and height
    w = x2 - x1;
    h = y2 - y1;
    // Jump back to bitmap coordinatess (adjusted)
    sx = x1 - data->xoffset;
    sy = y1 - data->yoffset;

    w *= data->bytesperpix;

    srcmod = data->bytesperline;
    dstmod = hwdata->bytesperline;

    src = data->VideoData + sy * data->bytesperline + sx * data->bytesperpix;
    dst = hwdata->framebuffer + y1 * hwdata->bytesperline + x1 * hwdata->bytesperpixel;

    // common sense assumption: memcpy can't possibly be faster than CopyMem[Quick]
    if ((srcmod != dstmod) || (srcmod != w))
    {
	for(y = 0; y < h; y++)
	{
	    CopyMem(src, dst, w);
	    src += srcmod;
	    dst += dstmod;
	}
    }
    else
    {
	// this is a plain total fast rulez copy
	CopyMem(src, dst, w*h);
    }*/
}
