/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: screen output functions.
*/

#include <aros/multiboot.h>

#include <bootconsole.h>
#include <stdarg.h>
#include <string.h>

#include "console.h"

/* Screen type */
unsigned char scr_Type = SCR_UNKNOWN;

/* Display buffer parameters */
void         *scr_FrameBuffer  = 0;	/* VRAM address			*/
unsigned int  scr_Width	       = 0;	/* Display width in characters	*/
unsigned int  scr_Height       = 0;	/* Display height in characters	*/
unsigned int  scr_BytesPerLine = 0;	/* Bytes per line		*/
unsigned int  scr_BytesPerPix  = 0;	/* Bytes per pixel		*/

/* Current output position (in characters) */
unsigned int scr_XPos = 0;
unsigned int scr_YPos = 0;

static unsigned char use_serial = 0;

void con_InitVESA(unsigned short version, struct vbe_mode *mode)
{
    scr_FrameBuffer  = (version >= 0x0200) ? (void *)mode->phys_base : 0;
    scr_BytesPerPix = mode->bits_per_pixel >> 3;

    if (mode->mode_attributes & VM_GRAPHICS)
    {
    	scr_Width  = mode->x_resolution / fontWidth;
    	scr_Height = mode->y_resolution / fontHeight;
    	scr_Type   = SCR_GFX;
    	scr_Mirror = boot_AllocMem(scr_Width * scr_Height);
    }
    else
    {
    	/* CHECKME: is it correct? It should fall back to VGA buffer address for text modes */
    	if (!scr_FrameBuffer)
    	    scr_FrameBuffer = (void *)((unsigned long)mode->win_a_segment << 16);
    	scr_Width = mode->x_resolution;
    	scr_Height = mode->y_resolution;
    	scr_Type = SCR_TEXT;
    }

    /* Use 3.0-specific field if available */
    if ((mode->mode_attributes & VM_LINEAR_FB) && (version >= 0x0300))
    	scr_BytesPerLine = mode->linear_bytes_per_scanline;
    else
    	scr_BytesPerLine = mode->bytes_per_scanline;

    /* We must have valid framebuffer address here */
    if (!scr_FrameBuffer)
        scr_Type = SCR_UNKNOWN;

    con_Clear();
}

void con_InitVGA(void)
{
    scr_FrameBuffer = (void *)0xb8000;
    scr_Width       = 80;
    scr_Height      = 25;
    scr_Type        = SCR_TEXT;
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
    scr_XPos = 0;
    scr_YPos = 0;

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
