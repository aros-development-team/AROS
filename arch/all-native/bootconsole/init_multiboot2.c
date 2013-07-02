/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Parse multiboot data and init console.
*/

#include <exec/types.h>
#include <aros/macros.h>
#include <aros/multiboot2.h>

#include <bootconsole.h>

#include "console.h"

void con_InitMultiboot2(void *mb)
{
    struct mb2_tag *tag;
    struct mb2_tag_framebuffer_common *fb = NULL;
    struct mb2_tag_vbe *vbe = NULL;

    /*
     * Iterate all tags and retrieve console information.
     * If we have command line supplied, we may also have serial console.
     * The supplied pointer points to an UQUAD value specifying total length of the
     * whole data array. We just skip it.
     * Size doesn't include padding needed to align the next tag at 8-byte boundary.
     */
    for (tag = mb + 8; tag->type != MB2_TAG_END; tag = (void *)tag + AROS_ROUNDUP2(tag->size, 8))
    {
    	switch (tag->type)
    	{
    	case MB2_TAG_CMDLINE:
    	    con_InitSerial(((struct mb2_tag_string *)tag)->string);
    	    break;

	case MB2_TAG_FRAMEBUFFER:
	    fb = (struct mb2_tag_framebuffer_common *)tag;
	    break;

	case MB2_TAG_VBE:
	    vbe = (struct mb2_tag_vbe *)tag;
	    break;
	}
    }

    if (fb)
    {
	/* Framebuffer was given, use it */
	scr_FrameBuffer = (void *)fb->framebuffer_addr;

	switch (fb->framebuffer_type)
	{
    	case MB2_FRAMEBUFFER_TEXT:
    	    /* Text framebuffer, size in characters */
	    scr_Width  = fb->framebuffer_width;
	    scr_Height = fb->framebuffer_height;
	    scr_Type   = SCR_TEXT;
	    txt_Clear();
	    break;

	default:
	    /* Graphical framebuffer, size in pixels */
	    scr_Type = SCR_GFX;
	    fb_Init(fb->framebuffer_width, fb->framebuffer_height, fb->framebuffer_bpp, fb->framebuffer_pitch);
	}
    }
    else if (vbe)
	con_InitVESA(vbe->vbe_control_info.version, &vbe->vbe_mode_info);
    else
    	/* Fallback to default, VGA text mode */
    	con_InitVGA();
}
