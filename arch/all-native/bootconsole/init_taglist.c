/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Parse AROS boot taglist and init console.
*/

#include <aros/kernel.h>
#include <hardware/vbe.h>
#include <utility/tagitem.h>
#include <proto/arossupport.h>

#include <bootconsole.h>

#include "console.h"

void con_InitTagList(const struct TagItem *tags)
{
    struct TagItem *tag;
    struct vbe_mode *vbemode = NULL;
    /* By default we have 2.0 data (framebuffer pointer filled in) */
    unsigned short vbever = 0x0200;

    while ((tag = LibNextTagItem((struct TagItem **)&tags)))
    {
	switch (tag->ti_Tag)
	{
	case KRN_CmdLine:
	    con_InitSerial((char *)tag->ti_Data);
	    break;

	case KRN_VBEModeInfo:
	    vbemode = (struct vbe_mode *)tag->ti_Data;
	    break;

	case KRN_VBEControllerInfo:
	    vbever = ((struct vbe_controller *)tag->ti_Data)->version;
	    break;
	}
    }

    if (vbemode)
	con_InitVESA(vbever, vbemode);
    else
    	con_InitVGA();
}
