/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <hardware/vbe.h>
#include <runtime.h>

#include <stdio.h>
#include <windows.h>

#include "bootstrap.h"
#include "winapi.h"

#define D(x) x

/* FIXME: Masks/shifts are preliminary and not tested */
static const char ModeTable[2][8] =
{
    {5, 0, 6, 5, 5, 11, 0, 0},
    {5, 0, 5, 5, 5, 10, 0, 0}
};

static int SetFormat(struct vbe_mode *info, unsigned char mode)
{
    if ((mode > 0) && (mode < FORMAT_OTHER))
    {
        mode--;

        memcpy(&info->red_mask_size, ModeTable[mode], 8);
        memcpy(&info->linear_red_mask_size, ModeTable[mode], 8);

        return 0;
    }
    else
    {
        DisplayError("Unknown display format %d\n"
                     "Screen size %d x %d, BPP %d, BytesPerLine %d",
                     mode, info->x_resolution, info->y_resolution, info->bits_per_pixel, info->bytes_per_scanline);
        
        return -1;
    }
}

int GetFBInfo(struct vbe_mode *info)
{
    HDC hdc;
    int ret;
    RawFrameBufferInfo fb;
    GXDeviceInfo dev;

    D(fprintf(stderr, "[Video] Getting framebuffer information...\n"));

    hdc = GetDC(NULL);
    ret = ExtEscape(hdc, GETRAWFRAMEBUFFER, 0, NULL, sizeof(fb), (char *)&fb);
    if (ret > 0)
    {
        ReleaseDC(NULL, hdc);

        D(fprintf(stderr, "[Video] GETRAWFRAMEBUFFER worked, addr %p, size %d x %d\n",
                  fb.pFramePointer, fb.cxPixels, fb.cyPixels));
        D(fprintf(stderr, "[Video] Format %d, BPP %d, BytesPerPixel %d, BytesPerLine %d\n",
                  fb.wFormat, fb.wBPP, fb.cxStride, fb.cyStride));

        info->bytes_per_scanline        = fb.cyStride;
        info->x_resolution              = fb.cxPixels;
        info->y_resolution              = fb.cyPixels;
        info->bits_per_pixel            = fb.wBPP;
        info->phys_base                 = (unsigned int)fb.pFramePointer; /* FIXME: Convert to physical! */
        info->linear_bytes_per_scanline = fb.cyStride;

        return SetFormat(info, fb.wFormat);
    }

    /*
     * HaRET also tries GAPI at this point. However:
     * 1. Some documentation says that GAPI is deprecated
     * 2. I don't want to bother with runtime linking, current code works quite fine.
     */

    dev.Version = 100;
    ret = ExtEscape(hdc, GETGXINFO, 0, NULL, sizeof(dev), (char *)&dev);
    ReleaseDC(NULL, hdc);
    if (ret > 0)
    {
        D(fprintf(stderr, "[Video] GETGXINFO worked, addr %p, size %ld x %ld\n",
                  dev.pvFrameBuffer, dev.cxWidth, dev.cyHeight));
        D(fprintf(stderr, "[Video] Format %ld, BPP %ld, BytesPerLine %ld\n",
                  dev.ffFormat, dev.cBPP, dev.cbStride));

        info->bytes_per_scanline        = dev.cbStride;
        info->x_resolution              = dev.cxWidth;
        info->y_resolution              = dev.cyHeight;
        info->bits_per_pixel            = dev.cBPP;
        info->phys_base                 = (unsigned int)dev.pvFrameBuffer; /* FIXME: Convert to physical! */
        info->linear_bytes_per_scanline = dev.cbStride;

        return SetFormat(info, dev.ffFormat);
    }

    DisplayError("Failed to get framebuffer information");
    return -1;
}
