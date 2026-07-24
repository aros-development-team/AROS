/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: VideoCore framebuffer gfx HIDD hardware support. The kernel already
          brings up the mailbox framebuffer (arch/aarch64-native/kernel/fb.c);
          this driver reuses that linear surface (both are linked into the one
          kickstart image, so the symbols resolve directly).
*/

#define DEBUG 0
#include <aros/debug.h>
#include <proto/exec.h>
#include <string.h>

#include "vcgfx_intern.h"
#include "vcgfx_hidd.h"

/* Provided by the kickstart kernel (fb.c). */
extern int krn_fb_init(unsigned int w, unsigned int h);
extern unsigned int krn_fb_width(void);
extern unsigned int krn_fb_height(void);
extern unsigned int krn_fb_pitch(void);
extern unsigned long long krn_fb_base(void);

BOOL initVCGfxHW(struct HWData *data)
{
    if (!krn_fb_base())
    {
        if (!krn_fb_init(640, 480))
        {
            D(bug("[VCGfx] HwInit: framebuffer not available\n"));
            return FALSE;
        }
    }

    data->framebuffer  = (APTR)(IPTR)krn_fb_base();
    data->width        = krn_fb_width();
    data->height       = krn_fb_height();
    data->bytesperline = krn_fb_pitch();
    data->depth        = 32;
    data->bitsperpixel = 32;
    data->bytesperpixel = 4;

    /*
     * The VideoCore surface displays byte order R,G,B,x (see fb.c put()), so
     * the pixel masks are red@0-7, green@8-15, blue@16-23. AROS's MAP_COLCOMP
     * shift convention is (mask << shift) == 0xFF000000, i.e. shift = 24 minus
     * the mask's bit position - NOT the bit position itself.
     */
    data->redmask   = 0x000000FF; data->redshift   = 24;
    data->greenmask = 0x0000FF00; data->greenshift = 16;
    data->bluemask  = 0x00FF0000; data->blueshift  = 8;
    data->palettewidth = 8;
    data->fbsize = data->height * data->bytesperline;

    D(bug("[VCGfx] HwInit: %ux%ux%u linear FB @ 0x%p, pitch %u\n",
          data->width, data->height, data->depth,
          data->framebuffer, data->bytesperline));

    ClearBuffer(data);
    return TRUE;
}

/* Copy the (possibly partial) bitmap buffer to the visible framebuffer. */
void vcfbDoRefreshArea(struct HWData *hwdata, struct VCGfxBitMapData *data,
                       LONG x1, LONG y1, LONG x2, LONG y2)
{
    UBYTE *src, *dst;
    ULONG srcmod, dstmod;
    LONG y, w, h, sx, sy;

    x1 += data->xoffset; y1 += data->yoffset;
    x2 += data->xoffset; y2 += data->yoffset;

    if ((x1 >= data->disp_width) || (x2 < 1) ||
        (y1 >= data->disp_height) || (y2 < 1))
        return;
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 > data->disp_width)  x2 = data->disp_width;
    if (y2 > data->disp_height) y2 = data->disp_height;

    w = x2 - x1;
    h = y2 - y1;
    sx = x1 - data->xoffset;
    sy = y1 - data->yoffset;
    w *= data->bytesperpix;

    srcmod = data->bytesperline;
    dstmod = hwdata->bytesperline;
    src = data->VideoData + sy * data->bytesperline + sx * data->bytesperpix;
    dst = (UBYTE *)hwdata->framebuffer + y1 * hwdata->bytesperline + x1 * hwdata->bytesperpixel;

    if ((srcmod != dstmod) || (srcmod != (ULONG)w))
    {
        for (y = 0; y < h; y++)
        {
            CopyMem(src, dst, w);
            src += srcmod;
            dst += dstmod;
        }
    }
    else
    {
        CopyMem(src, dst, w * h);
    }
}

/* Truecolor only: no hardware palette to load. */
void DACLoad(struct VCGfx_staticdata *xsd, UBYTE *DAC, unsigned char first, int num)
{
    (void)xsd; (void)DAC; (void)first; (void)num;
}

void ClearBuffer(struct HWData *data)
{
    if (data->framebuffer)
        memset(data->framebuffer, 0, data->height * data->bytesperline);
}
