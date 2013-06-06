/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

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

#include <stddef.h>
#include <string.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"
#include "dispinfo.h"

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

/* Looks up a DriverData corresponding to a MonitorSpec */
struct monitor_driverdata *MonitorFromSpec(struct MonitorSpec *mspc, struct GfxBase *GfxBase)
{
    struct monitor_driverdata *ret = NULL;
    struct monitor_driverdata *mdd;
    OOP_Object *drv;

    if (!mspc)
    	return NULL;

    /*
     * FIXME: NULL ms_Object will likely mean chipset MonitorSpec (they don't have 1:1 relation with sync objects)
     * Process this correctly here. Or am i wrong ?
     */
    if (!mspc->ms_Object)
    	 return NULL;

    OOP_GetAttr((OOP_Object *)mspc->ms_Object, aHidd_Sync_GfxHidd, (IPTR *)&drv);

    ObtainSemaphoreShared(&CDD(GfxBase)->displaydb_sem);

    for (mdd = CDD(GfxBase)->monitors; mdd; mdd = mdd->next)
    {
	/*
	 * Sync objects know nothing about fakegfx proxy class.
	 * They carry a pointer to a real driver object.
	 */
    	if (mdd->gfxhidd_orig == drv)
    	{
    	    ret = mdd;
    	    break;
    	}
    }

    ReleaseSemaphore(&CDD(GfxBase)->displaydb_sem);
    
    return ret;
}
