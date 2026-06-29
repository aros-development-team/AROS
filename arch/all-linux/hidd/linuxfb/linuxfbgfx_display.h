#ifndef LINUXFBGFX_DISPLAY_H
#define LINUXFBGFX_DISPLAY_H

/*
    Copyright (C) 2016-2026, The AROS Development Team. All rights reserved.

    Desc: LinuxFB Gfx display class data.
    Lang: English.
*/

#include <exec/types.h>

#define CLID_Hidd_Display_LinuxFB	"hidd.display.linuxfb"
#define IID_Hidd_Display_LinuxFB	"hidd.display.linuxfb"

struct LinuxFBDisplayData
{
    /* baseclass for CreateObject */
    OOP_Class		*basebm;

    struct FBDevInfo	*fbdevinfo;	/* Points to the gfx object's FBDevInfo */
    BOOL		gamma;
    UWORD		scale_size;
    UBYTE		r_step;
    UBYTE		g_step;
    UBYTE		b_step;
};

#endif /* LINUXFBGFX_DISPLAY_H */
