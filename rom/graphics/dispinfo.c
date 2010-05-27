/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/arossupport.h>
#include <proto/utility.h>
#include <proto/graphics.h>

#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/rawfmt.h>

#include <graphics/displayinfo.h>
#include <graphics/monitor.h>

#include <cybergraphx/cybergraphics.h>

#include <oop/oop.h>

#include <hidd/graphics.h>

#include <stdio.h>
#include <string.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"
#include "dispinfo.h"

#define DEBUG 0
#include <aros/debug.h>

HIDDT_ModeID get_best_resolution_and_depth(struct monitor_driverdata *mdd, struct GfxBase *GfxBase)
{
    HIDDT_ModeID ret = vHidd_ModeID_Invalid;
    struct DisplayInfoHandle *dh;
    ULONG best_resolution = 0;
    ULONG best_depth = 0;

    for (dh = mdd->modes; dh->id != vHidd_ModeID_Invalid; dh++) {
	OOP_Object *sync, *pf;
	IPTR depth;

	HIDD_Gfx_GetMode(mdd->gfxhidd, dh->id, &sync, &pf);
	OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);

	if (depth >= best_depth) {
	    IPTR width, height;
	    ULONG res;

	    OOP_GetAttr(sync, aHidd_Sync_HDisp, &width);
	    OOP_GetAttr(sync, aHidd_Sync_VDisp, &height);

	    res = width * height;
	    if (res > best_resolution) {
		ret = dh->id;
                best_resolution = res;
	    }
	    best_depth = depth;
	}
    }
    return ret;
}
