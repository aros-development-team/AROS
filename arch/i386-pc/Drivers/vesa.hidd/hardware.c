/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
 
    Desc: vesa "hardware" functions
    Lang: English
*/


#define DEBUG 1 /* no SysBase */
#include <aros/debug.h>
#include <aros/macros.h>
#include <aros/bootloader.h>
#include <proto/bootloader.h>

#include "bitmap.h"
#include "vesagfxclass.h"
#include "hardware.h"

#include <string.h>

#undef SysBase
extern struct ExecBase *SysBase;

BOOL initVesaGfxHW(struct HWData *data)
{
    struct BootLoaderBase *BootLoaderBase;
    struct VesaInfo *vi;

    if ((BootLoaderBase = OpenResource("bootloader.resource")))
    {
	D(bug("[Vesa] Init: Bootloader.resource opened\n"));
	if ((vi = (struct VesaInfo *)GetBootInfo(BL_Video)))
	{
	    D(bug("[Vesa] Init: Got Vesa structure from resource\n"));
	    if (vi->ModeNumber == 3)
	    {
		D(bug("[Vesa] Init: Textmode was specified. Aborting\n"));
		return FALSE;
	    }
	    data->width = vi->XSize; data->height = vi->YSize;
	    data->bitsperpixel = data->depth = vi->BitsPerPixel;
	    data->bytesperline = vi->BytesPerLine;
	    data->redmask = vi->Masks[VI_Red];
	    data->greenmask = vi->Masks[VI_Green];
	    data->bluemask = vi->Masks[VI_Blue];
	    data->redshift = vi->Shifts[VI_Red];
	    data->greenshift = vi->Shifts[VI_Green];
	    data->blueshift = vi->Shifts[VI_Blue];
	    data->framebuffer = vi->FrameBuffer;
	    
	    if (data->depth > 24)
	    {
	    	data->bytesperpixel = 4;
	    }
	    else if (data->depth > 16)
	    {
	    	data->bytesperpixel = 3;
	    }
	    else if (data->depth > 8)
	    {
	    	data->bytesperpixel = 2;
	    }
	    else
	    {
	    	data->bytesperpixel = 1;
	    }
	    
	    D(bug("[Vesa] HwInit: Clearing framebuffer at 0x%08x size %d KB\n",data->framebuffer, vi->FrameBufferSize));
	    memset(data->framebuffer, 0, vi->FrameBufferSize * 1024);
	    D(bug("[Vesa] HwInit: Linear framebuffer at 0x%08x\n",data->framebuffer));
	    D(bug("[Vesa] HwInit: Screenmode %dx%dx%d\n",data->width,data->height,data->depth));
	    D(bug("[Vesa] HwInit: Masks R %08x<<%2d G %08x<<%2d B %08x<<%2d\n",
			data->redmask, data->redshift,
			data->greenmask, data->greenshift,
			data->bluemask, data->blueshift));
	    D(bug("[vesa] HwInit: BytesPerPixel %d\n", data->bytesperpixel));
	    return TRUE;
	}
    }

    bug("[Vesa] HwInit: No Vesa information from the bootloader. Failing\n");
    return FALSE;
}


#if BUFFERED_VRAM
void vesaRefreshArea(struct BitmapData *data, LONG x1, LONG y1, LONG x2, LONG y2)
{
    UBYTE *src, *dst;
    ULONG srcmod, dstmod;
    LONG x, y, w, h;

    x1 *= data->bytesperpix;
    x2 *= data->bytesperpix; x2 += data->bytesperpix - 1;
    
    x1 &= ~3;
    x2 = (x2 & ~3) + 3;
    w = (x2 - x1) + 1;
    h = (y2 - y1) + 1;
    
    srcmod = (data->bytesperline - w);
    dstmod = (data->data->bytesperline - w);
   
    src = data->VideoData + y1 * data->bytesperline + x1;
    dst = data->data->framebuffer + y1 * data->data->bytesperline + x1;
    
    for(y = 0; y < h; y++)
    {
    	for(x = 0; x < w / 4; x++)
	{
	    *((ULONG *)dst) = *((ULONG *)src);
	    dst += 1;
	    src += 1;
	}
	src += srcmod;
	dst += dstmod;
    }
    
}
#endif
