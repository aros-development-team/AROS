/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Linear framebuffer gfx HIDD. The bootstrap brings the VideoCore
          framebuffer up and hands its geometry to the kernel; this driver
          wraps that surface, querying it via KrnGetSystemAttr. It touches no
          VideoCore register itself - the one board-specific thing left is the
          pixel byte order below, which the firmware fixes and no attribute
          reports.
*/

#define DEBUG 0
#include <aros/debug.h>
#include <aros/kernel.h>
#include <proto/exec.h>

/* kernel.resource is a resource, not a library: open it explicitly rather
   than letting the module's autoinit try to OpenLibrary() it. */
#define __NOLIBBASE__
#include <proto/kernel.h>
#include <string.h>

#include "fbgfx_intern.h"
#include "fbgfx_hidd.h"
#include "fbgfx_refresh.h"

BOOL initFBGfxHW(struct HWData *data)
{
    struct KernelBase *KernelBase = OpenResource("kernel.resource");
    IPTR fb = KernelBase ? (IPTR)KrnGetSystemAttr(KATTR_FrameBuffer) : 0;
    IPTR width, height, pitch, depth;
    uint32_t bytes;

    /* KrnGetSystemAttr() answers -1 for anything it does not know, so a
       kernel without the framebuffer attributes hands back a pointer that
       is not NULL but is not memory either. */
    if (fb == 0 || fb == (IPTR)-1)
    {
        D(bug("[FBGfx] HwInit: framebuffer not available\n"));
        return FALSE;
    }

    width = KrnGetSystemAttr(KATTR_FrameBufferWidth);
    height = KrnGetSystemAttr(KATTR_FrameBufferHeight);
    pitch = KrnGetSystemAttr(KATTR_FrameBufferPitch);
    depth = KrnGetSystemAttr(KATTR_FrameBufferDepth);
    if (depth != 32 || width > UINT32_MAX || height > UINT32_MAX || pitch > UINT32_MAX ||
        !fbgfx_buffer_size(width, height, pitch, 4, &bytes) ||
        (uintptr_t)fb > UINTPTR_MAX - bytes)
    {
        D(bug("[FBGfx] HwInit: invalid framebuffer geometry\n"));
        return FALSE;
    }

#if !defined(DEBUGDISPLAY)
    /*
     * Detach the bootstrap's framebuffer console before handing this surface
     * on. 0x03 to RawPutChar drops ARMI_PutChar (krnPutC in
     * arch/aarch64-native/kernel/kernel_debug.c), leaving debug output on the
     * serial line; without it every bug() keeps drawing characters into the
     * screen Workbench is using.
     */
    RawPutChar(0x03);
#endif

    data->framebuffer  = (APTR)fb;
    data->width        = width;
    data->height       = height;
    data->bytesperline = pitch;
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
    data->fbsize = bytes;

    D(bug("[FBGfx] HwInit: %ux%ux%u linear FB @ 0x%p, pitch %u\n",
          data->width, data->height, data->depth,
          data->framebuffer, data->bytesperline));

    ClearBuffer(data);
    return TRUE;
}

/*
 * Copy the (possibly partial) bitmap area [x1, x2) x [y1, y2), in bitmap
 * coordinates, to the visible framebuffer. Truecolor bitmaps are copied
 * as is, LUT8 bitmaps are expanded through their software palette.
 */
void fbDoRefreshArea(struct HWData *hwdata, struct FBGfxBitMapData *data,
                       LONG x1, LONG y1, LONG x2, LONG y2)
{
    FBGfxSurface src, dst;
    ULONG dst_width, dst_height;
    uint32_t src_bytes;

    if (!hwdata || !data)
        return;
    if ((data->width <= 0) || (data->height <= 0) ||
        (data->disp_width <= 0) || (data->disp_height <= 0))
        return;
    /* The framebuffer is RGBX, which is all the helper writes */
    if (hwdata->bytesperpixel != 4)
        return;
    if (!fbgfx_buffer_size(data->width, data->height, data->bytesperline,
                           data->bytesperpix, &src_bytes))
        return;

    src.pixels          = data->VideoData;
    src.bytes           = src_bytes;
    src.width           = (uint32_t)data->width;
    src.height          = (uint32_t)data->height;
    src.pitch           = data->bytesperline;
    src.bytes_per_pixel = data->bytesperpix;

    /* Never draw outside either the physical surface or the display mode */
    dst_width  = hwdata->width;
    dst_height = hwdata->height;
    if ((ULONG)data->disp_width < dst_width)
        dst_width = data->disp_width;
    if ((ULONG)data->disp_height < dst_height)
        dst_height = data->disp_height;

    dst.pixels          = hwdata->framebuffer;
    dst.bytes           = hwdata->fbsize;
    dst.width           = dst_width;
    dst.height          = dst_height;
    dst.pitch           = hwdata->bytesperline;
    dst.bytes_per_pixel = 4;

    fbgfx_refresh_rect(&src, &dst, data->DAC, data->xoffset, data->yoffset,
                       x1, y1, x2, y2);
}

/*
 * The hardware is truecolor only: there is no palette to load. LUT8
 * bitmaps are converted in software by fbDoRefreshArea().
 */
void DACLoad(struct FBGfx_staticdata *xsd, UBYTE *DAC, unsigned char first, int num)
{
    (void)xsd; (void)DAC; (void)first; (void)num;
}

void ClearBuffer(struct HWData *data)
{
    if (data->framebuffer)
        memset(data->framebuffer, 0, data->fbsize);
}
