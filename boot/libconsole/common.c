/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Common console output functions.
*/

#include <aros/multiboot.h>

#include <bootconsole.h>
#include <stdarg.h>
#include <string.h>

#include "console.h"

/* Screen type */
unsigned char scr_Type = SCR_UNKNOWN;

static unsigned char use_serial = 0;

void con_InitVESA(unsigned short version, struct vbe_mode *mode)
{
    scr_FrameBuffer = (version >= 0x0200) ? (void *)mode->phys_base : 0;

    if (mode->mode_attributes & VM_GRAPHICS)
    {
        unsigned int pitch;

        scr_Type = SCR_GFX;

	/* Use 3.0-specific field if available */
	if ((mode->mode_attributes & VM_LINEAR_FB) && (version >= 0x0300))
	    pitch = mode->linear_bytes_per_scanline;
	else
	    pitch = mode->bytes_per_scanline;

        fb_Init(mode->x_resolution, mode->y_resolution, mode->bits_per_pixel, pitch);
    }
    else
    {
        scr_Type   = SCR_TEXT;
        scr_Width  = mode->x_resolution;
    	scr_Height = mode->y_resolution;

    	/* CHECKME: is it correct? It should fall back to VGA buffer address for text modes */
    	if (!scr_FrameBuffer)
    	    scr_FrameBuffer = (void *)((unsigned long)mode->win_b_segment << 16);

	txt_Clear();
    }

    /* We must have valid framebuffer address here */
    if (!scr_FrameBuffer)
        scr_Type = SCR_UNKNOWN;
}

void con_InitVGA(void)
{
    scr_Type        = SCR_TEXT;
    scr_FrameBuffer = (void *)0xb8000;
    scr_Width       = 80;
    scr_Height      = 25;

    txt_Clear();
}

void con_InitSerial(char *cmdline)
{
    char *opts = strstr(cmdline, "debug=serial");
    
    if (opts)
    {
    	use_serial = 1;
    	
    	serial_Init(&cmdline[12]);
    }
    else
    	use_serial = 0;
}

void con_Clear()
{
    switch (scr_Type)
    {
    case SCR_TEXT:
    	txtClear();
    	break;
    
    case SCR_GFX:
    	gfxClear();
    	break;
    }
}

void con_Putc(char c)
{
    if (use_serial)
    {
    	if (c == '\n')
	    serial_Putc('\r');
	serial_Putc(c);
    }

    switch (scr_Type)
    {
    case SCR_TEXT:
    	txtPutc(c);
    	break;

    case SCR_GFX:
    	gfxPutc(c);
    	break;
    }
}
