/*
    Copyright � 2013-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include "vc4gfx_hidd.h"

int FNAME_SUPPORT(SDTV_SyncGen)(struct List *modelist, OOP_Class *cl)
{
    struct DisplayMode *sdtv_mode;
    int sdtv_modecount = 0;
    BOOL sdtv_active = FALSE;

    D(bug("[VideoCoreGfx] %s()\n", __PRETTY_FUNCTION__));

//#warning "TODO: check if an SDTV is attached"
    if (sdtv_active)
    {
        if ((sdtv_mode = AllocMem(sizeof(struct DisplayMode), MEMF_PUBLIC)) != NULL)
        {
            sdtv_mode->dm_clock = 25174;
            sdtv_mode->dm_hdisp = 640;
            sdtv_mode->dm_hstart = 656;
            sdtv_mode->dm_hend = 752;
            sdtv_mode->dm_htotal = 800;
            sdtv_mode->dm_vdisp = 480;
            sdtv_mode->dm_vstart = 490;
            sdtv_mode->dm_vend = 492;
            sdtv_mode->dm_vtotal = 525;
            sdtv_mode->dm_descr = "VideoCore: PAL 640x480";
            AddTail(modelist, &sdtv_mode->dm_Node);
            sdtv_modecount++;
        }

        if ((sdtv_mode = AllocMem(sizeof(struct DisplayMode), MEMF_PUBLIC)) != NULL)
        {
            sdtv_mode->dm_clock = 25174;
            sdtv_mode->dm_hdisp = 640;
            sdtv_mode->dm_hstart = 656;
            sdtv_mode->dm_hend = 752;
            sdtv_mode->dm_htotal = 800;
            sdtv_mode->dm_vdisp = 400;
            sdtv_mode->dm_vstart = 410;
            sdtv_mode->dm_vend = 412;
            sdtv_mode->dm_vtotal = 445;
            sdtv_mode->dm_descr = "VideoCore: NTSC 640x400";
            AddTail(modelist, &sdtv_mode->dm_Node);
            sdtv_modecount++;
        }
    }
    return sdtv_modecount;
}
