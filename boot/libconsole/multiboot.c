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
	scr_FrameBuffer  = (void *)(unsigned long)mb->framebuffer_addr;
	scr_BytesPerLine = mb->framebuffer_pitch;
	scr_BytesPerPix  = mb->framebuffer_bpp >> 3;

    	switch (mb->framebuffer_type)
    	{
    	case MB_FRAMEBUFFER_TEXT:
    	   /* Text framebuffer, size in characters */
	   scr_Width  = mb->framebuffer_width;
	   scr_Height = mb->framebuffer_height;
	   scr_Type   = SCR_TEXT;
	   scr_Mirror = boot_AllocMem(scr_Width * scr_Height);
	   break;

	default:
	   /* Graphical framebuffer, size in pixels */
	   scr_Width  = mb->framebuffer_width  / fontWidth;
	   scr_Height = mb->framebuffer_height / fontHeight;
	   scr_Type   = SCR_GFX;
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

    con_Clear();
}
