/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
 
    Desc: vesa "hardware" functions
    Lang: English
*/


#define DEBUG 1 /* no SysBase */
#include <aros/debug.h>
#include <aros/macros.h>

#include "hardware.h"
#include "vesagfxclass.h"
#include "multiboot.h"

#undef SysBase
extern struct ExecBase *SysBase;

BOOL initVesaGfxHW(struct HWData *data)
{
    struct arosmb *mb;
    ULONG masks [] = { 0x01, 0x03, 0x07, 0x0f ,0x1f, 0x3f, 0x7f, 0xff };

    mb = (struct arosmb *)0x1000;

    data->width = mb->vmi.x_resolution;
    data->height = mb->vmi.y_resolution;
    data->depth = mb->vmi.bits_per_pixel;
    data->redmask    = masks[mb->vmi.red_mask_size-1]<<mb->vmi.red_field_position;
    data->greenmask  = masks[mb->vmi.green_mask_size-1]<<mb->vmi.green_field_position;
    data->bluemask   = masks[mb->vmi.blue_mask_size-1]<<mb->vmi.blue_field_position;
    data->redshift   = 32 - mb->vmi.red_field_position - mb->vmi.red_mask_size;
    data->greenshift = 32 - mb->vmi.green_field_position - mb->vmi.green_mask_size;
    data->blueshift  = 32 - mb->vmi.blue_field_position - mb->vmi.blue_mask_size;
    data->bytesperpixel = 1;
    if (data->depth>16)
	data->bytesperpixel = 4;
    else if (data->depth>8)
	data->bytesperpixel = 2;
    data->bitsperpixel = mb->vmi.bits_per_pixel;
    data->bytesperline = mb->vmi.bytes_per_scanline;
    data->fbsize = ((mb->vci.total_memory)<<6)*1024;
    data->framebuffer = mb->vmi.phys_base;

    D(bug("[Vesa] HwInit: Linear framebuffer at 0x%08x\n",data->framebuffer));
    D(bug("[Vesa] HwInit: Screenmode %dx%dx%d\n",mb->vmi.x_resolution,mb->vmi.y_resolution,data->depth));
    D(bug("[Vesa] HwInit: Masks R %08x<<%2d G %08x<<%2d B %08x<<%2d\n",
		data->redmask, data->redshift,
		data->greenmask, data->greenshift,
		data->bluemask, data->blueshift));

    return TRUE;
}
