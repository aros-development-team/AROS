/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: common.c 37743 2011-03-23 13:53:43Z sonic $

    Desc: Parse multiboot data and init console.
*/

#include <aros/multiboot.h>
#include <hardware/vbe.h>

#include <bootconsole.h>

#include "console.h"

void con_InitMultiboot(struct multiboot *mb)
{
    if (mb->flags & MB_FLAGS_CMDLINE)
    	con_InitSerial((char *)(unsigned long)mb->cmdline);

    if (mb->flags & MB_FLAGS_FB)
    {
    	/* Framebuffer was given, use it */
	scr_FrameBuffer = (void *)(unsigned long)mb->framebuffer_addr;

    	switch (mb->framebuffer_type)
    	{
    	case MB_FRAMEBUFFER_TEXT:
    	   /* Text framebuffer, size in characters */
	   scr_Width  = mb->framebuffer_width;
	   scr_Height = mb->framebuffer_height;
	   scr_Type   = SCR_TEXT;
	   txt_Clear();
	   break;

	default:
	   /* Graphical framebuffer, size in pixels */
	   scr_Type = SCR_GFX;
	   fb_Init(mb->framebuffer_width, mb->framebuffer_height, mb->framebuffer_bpp, mb->framebuffer_pitch);
	}
    }
/*
 * TODO: enable this only after testing text mode handling in initVESAScreen() below.
 * VGA text mode may have valid VESA mode descriptor. If there is a bug in that routine,
 * text mode display will screw up, and probably crash.
 *
    else if (mb->flags & MB_FLAGS_GFX)
    {
    	struct vbe_control_info *vbc = (struct vbe_control_info *)mb->vbe_control_info;
    	struct vbe_mode_info    *vbm = (struct vbe_mode_info *)mb->vbe_mode_info;

    	con_InitVESA(vbc->version, vbm);
    }*/
    else
        /* Fallback to default, VGA text mode */
    	con_InitVGA();
}
