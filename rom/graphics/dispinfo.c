/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
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

#include <hidd/gfx.h>

#include <stddef.h>
#include <string.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"
#include "dispinfo.h"

HIDDT_ModeID get_best_resolution_and_depth(struct monitor_displaydata *mdd, struct GfxBase *GfxBase)
{
    HIDDT_ModeID ret = vHidd_ModeID_Invalid;
    struct DisplayInfoHandle *dh;
    ULONG best_resolution = 0;
    ULONG best_depth = 0;

    for(dh = mdd->modes; dh->id != vHidd_ModeID_Invalid; dh++) {
        OOP_Object *sync, *pf;
        IPTR depth;

        HIDD_DMEnum_GetMode(mdd->mdisplay.display_dmenum, dh->id, &sync, &pf);
        OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);

        if(depth >= best_depth) {
            IPTR width, height;
            ULONG res;

            OOP_GetAttr(sync, aHidd_Sync_HDisp, &width);
            OOP_GetAttr(sync, aHidd_Sync_VDisp, &height);

            res = width * height;
            if(res > best_resolution) {
                ret = dh->id;
                best_resolution = res;
            }
            best_depth = depth;
        }
    }
    return ret;
}

/* Looks up a DriverData corresponding to a MonitorSpec */
struct monitor_displaydata *MonitorFromSpec(struct MonitorSpec *mspc, struct GfxBase *GfxBase)
{
    struct monitor_displaydata *ret = NULL;
    struct monitor_displaydata *mdd;
    OOP_Object *dmenum;

    if(!mspc)
        return NULL;

    /*
     * FIXME: NULL ms_Object will likely mean chipset MonitorSpec (they don't have 1:1 relation with sync objects)
     * Process this correctly here. Or am i wrong ?
     */
    if(!mspc->ms_Object)
        return NULL;

    D(bug("[graphics.library] %s: driver display @ 0x%p\n", __func__, mspc->ms_Object));

    OOP_GetAttr((OOP_Object *)mspc->ms_Object, aHidd_Sync_DMEnumerator, (IPTR *)&dmenum);
    D(bug("[graphics.library] %s: dmenum @ 0x%p\n", __func__, dmenum));

    ObtainSemaphoreShared(&CDD(GfxBase)->displaydb_sem);

    for(mdd = GFXPRIVATE_MONITORFIRST; mdd; mdd = (struct monitor_displaydata *)mdd->mdisplay.display_next) {
        /*
         * Sync objects carry a pointer to the display mode enumerator
         * of the display they belong to.
         */
        if(mdd->mdisplay.display_dmenum == dmenum) {
            ret = mdd;
            break;
        }
    }

    ReleaseSemaphore(&CDD(GfxBase)->displaydb_sem);

    return ret;
}
