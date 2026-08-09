/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: EFI framebuffer Gfx hardware support. The firmware (GOP) set the
          mode and handed over a linear framebuffer; the boot code passed
          its description through the KRN_VBEModeInfo/KRN_FBAddr tags and
          bootloader.resource republishes it as BL_Video. There is no
          hardware to program - everything here is reading that record
          and copying pixels.
*/

#define DEBUG 0
#include <aros/debug.h>
#include <aros/bootloader.h>
#include <proto/bootloader.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include "efigfx_intern.h"
#include "efigfx_hidd.h"

BOOL initEFIGfxHW(struct HWData *data)
{
    struct BootLoaderBase *BootLoaderBase;
    struct VesaInfo *vi;

    if (!(BootLoaderBase = OpenResource("bootloader.resource")))
        return FALSE;

    if (!(vi = (struct VesaInfo *)GetBootInfo(BL_Video)))
    {
        D(bug("[EFIGfx] HwInit: no video information from the bootloader\n"));
        return FALSE;
    }

    if (!vi->FrameBuffer)
        return FALSE;

    data->width = vi->XSize;
    data->height = vi->YSize;
    data->bitsperpixel = data->depth = vi->BitsPerPixel;
    data->bytesperline = vi->BytesPerLine;
    if (data->depth > 8)
    {
        data->redmask = vi->Masks[VI_Red];
        data->greenmask = vi->Masks[VI_Green];
        data->bluemask = vi->Masks[VI_Blue];
        data->redshift = vi->Shifts[VI_Red];
        data->greenshift = vi->Shifts[VI_Green];
        data->blueshift = vi->Shifts[VI_Blue];
    }
    else
    {
        /* Fake masks to allow this mode to be displayed */
        data->redmask = 0xff0000;
        data->greenmask = 0xff00;
        data->bluemask = 0xff;
        data->redshift = 16;
        data->greenshift = 8;
        data->blueshift = 0;
    }
    data->framebuffer = vi->FrameBuffer;
    data->palettewidth = vi->PaletteWidth;
    if (data->palettewidth > 8)
        data->palettewidth = 8;

    if (data->depth > 24)
        data->bytesperpixel = 4;
    else if (data->depth > 16)
        data->bytesperpixel = 3;
    else if (data->depth > 8)
        data->bytesperpixel = 2;
    else
        data->bytesperpixel = 1;

    data->fbsize = data->height * data->bytesperline;

    D(bug("[EFIGfx] HwInit: %ux%ux%u linear FB @ 0x%p, pitch %u\n",
          data->width, data->height, data->depth,
          data->framebuffer, data->bytesperline));
    D(bug("[EFIGfx] HwInit: Masks R %08x<<%2d G %08x<<%2d B %08x<<%2d\n",
          data->redmask, data->redshift,
          data->greenmask, data->greenshift,
          data->bluemask, data->blueshift));

    ClearBuffer(data);
    return TRUE;
}

/* Copy the (possibly partial) bitmap buffer to the visible framebuffer. */
void efiDoRefreshArea(struct HWData *hwdata, struct EFIGfxBitMapData *data,
                      LONG x1, LONG y1, LONG x2, LONG y2)
{
    UBYTE *src, *dst;
    ULONG srcmod, dstmod;
    LONG y, w, h, sx, sy;

    x1 += data->xoffset; y1 += data->yoffset;
    x2 += data->xoffset; y2 += data->yoffset;

    /* Clip the rectangle against physical display borders */
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

/* The GOP framebuffer is truecolor: no hardware palette to load. */
void DACLoad(struct EFIGfx_staticdata *xsd, UBYTE *DAC, unsigned char first, int num)
{
    (void)xsd; (void)DAC; (void)first; (void)num;
}

void ClearBuffer(struct HWData *data)
{
    if (data->framebuffer)
        SetMem(data->framebuffer, 0, data->height * data->bytesperline);
}
