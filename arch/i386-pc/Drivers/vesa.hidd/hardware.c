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

#include "hardware.h"
#include "vesagfxclass.h"

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
	    data->bytesperpixel = 1;
	    if (data->depth>16)
		data->bytesperpixel = 4;
	    else if (data->depth>8)
		data->bytesperpixel = 2;
	    D(bug("[Vesa] HwInit: Linear framebuffer at 0x%08x\n",data->framebuffer));
	    D(bug("[Vesa] HwInit: Screenmode %dx%dx%d\n",data->width,data->height,data->depth));
	    D(bug("[Vesa] HwInit: Masks R %08x<<%2d G %08x<<%2d B %08x<<%2d\n",
			data->redmask, data->redshift,
			data->greenmask, data->greenshift,
			data->bluemask, data->blueshift));
	    return TRUE;
	}
    }

    bug("[Vesa] HwInit: No Vesa information from the bootloader. Failing\n");
    return FALSE;
}
