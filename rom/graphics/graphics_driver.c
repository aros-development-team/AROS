/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Driver for using gfxhidd for gfx output
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/arossupport.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <exec/memory.h>
#include <exec/semaphores.h>
#include <clib/macros.h>

#include <graphics/rastport.h>
#include <graphics/gfxbase.h>
#include <graphics/text.h>
#include <graphics/view.h>
#include <graphics/layers.h>
#include <graphics/clip.h>
#include <graphics/gfxmacros.h>
#include <graphics/regions.h>
#include <graphics/scale.h>

#include <oop/oop.h>
#include <utility/tagitem.h>
#include <aros/asmcall.h>

#include <hidd/compositor.h>
#include <hidd/gfx.h>

#include <cybergraphx/cybergraphics.h>

#include <stdio.h>
#include <string.h>

#include "graphics_intern.h"
#include "graphics_driver.h"
#include "graphics_display.h"
#include "graphics_compositor.h"
#include "intregions.h"
#include "dispinfo.h"
#include "gfxfuncsupport.h"
#include "fontsupport.h"

#define DEBUG_INIT(x)
#define DEBUG_LOADVIEW(x)

/* Define this if you wish to enforce using software mouse sprite
   even for drivers that support hardware one. Useful for debugging
   and testing
#define FORCE_SOFTWARE_SPRITE */

struct ETextFont {
    struct TextFont     etf_Font;
};

/* *********************** RastPort extra data handling *********************** */

struct gfx_driverdata *AllocDriverData(struct RastPort *rp, BOOL alloc, struct GfxBase *GfxBase)
{
    struct gfx_driverdata *dd = ObtainDriverData(rp);

    if(alloc && !dd) {
        dd = AllocVec(sizeof(struct gfx_driverdata), MEMF_CLEAR);
        if(dd) {
            rp->RP_Extra    = dd;
            dd->dd_RastPort = rp;
        }
    }

    return dd;
}

/* *********************** Display driver handling *********************** */

int driver_init(struct GfxBase * GfxBase)
{
    D(bug("[graphics.library/driver] %s()\n", __func__));

    /* Our underlying RTG subsystem core must be already up and running */
    if (!OpenLibrary("gfx.hidd", 0))
        return FALSE;

    /* Initialize the semaphores */
    InitSemaphore(&(PrivGBase(GfxBase)->blit_sema));

    /* Init the needed attrbases */
    __IHidd_Gfx      = OOP_ObtainAttrBase(IID_Hidd_Gfx);
    __IHidd_Display  = OOP_ObtainAttrBase(IID_Hidd_Display);
    __IHidd_DMEnum   = OOP_ObtainAttrBase(IID_Hidd_DMEnum);
    __IHidd_BitMap   = OOP_ObtainAttrBase(IID_Hidd_BitMap);
    __IHidd_GC       = OOP_ObtainAttrBase(IID_Hidd_GC);
    __IHidd_Sync     = OOP_ObtainAttrBase(IID_Hidd_Sync);
    __IHidd_PixFmt   = OOP_ObtainAttrBase(IID_Hidd_PixFmt);
    __IHidd_PlanarBM = OOP_ObtainAttrBase(IID_Hidd_PlanarBM);

    if (__IHidd_Gfx && __IHidd_Display && __IHidd_DMEnum && __IHidd_BitMap &&
        __IHidd_GC && __IHidd_Sync && __IHidd_PixFmt && __IHidd_PlanarBM)
    {
        OOP_Class *baseGfx;

        CDD(GfxBase)->gcClass = OOP_FindClass(CLID_Hidd_GC);
        if (!CDD(GfxBase)->gcClass)
            return FALSE;

        /* Init display mode database */
        InitSemaphore(&CDD(GfxBase)->displaydb_sem);
        CDD(GfxBase)->mdisplay.display_idbase    = INVALID_ID;
        CDD(GfxBase)->mdisplay.display_mask      = AROS_RTG_MONITOR_ID;
        CDD(GfxBase)->mdisplay.display_compositor = NULL;
        CDD(GfxBase)->mdisplay.display_dmenum     = NULL;

        /* Init software driver, added as a driver of the gfx HW object */
        baseGfx = OOP_FindClass(CLID_Hidd_Gfx);
        CDD(GfxBase)->mdisplay.display_gfxhidd = HW_AddDriver(PrivGBase(GfxBase)->GfxRoot, baseGfx, NULL);

        if (CDD(GfxBase)->mdisplay.display_gfxhidd)
        {
            struct TagItem display_tags[] =
            {
                { aHidd_Display_GfxHidd, (IPTR)CDD(GfxBase)->mdisplay.display_gfxhidd },
                { TAG_DONE,              0                                            }
            };
            CDD(GfxBase)->mdisplay.display_obj = OOP_NewObject(NULL, CLID_Hidd_Display, display_tags);

            if (CDD(GfxBase)->mdisplay.display_obj)
            {
                struct TagItem bm_create_tags[] =
                {
                    { aHidd_BitMap_Display,  (IPTR)CDD(GfxBase)->mdisplay.display_obj },
                    { aHidd_PlanarBM_BitMap, 0                                        },
                    { TAG_DONE,              0                                        }
                };

                CDD(GfxBase)->planarbm_cache = create_object_cache(NULL, CLID_Hidd_PlanarBM, bm_create_tags, GfxBase);
                if (CDD(GfxBase)->planarbm_cache)
                {
                    CDD(GfxBase)->gc_cache = create_object_cache(NULL, CLID_Hidd_GC, NULL, GfxBase);
                    if (CDD(GfxBase)->gc_cache)
                    {
                        D(bug("[graphics.library/driver] %s: initialized\n", __func__));
                        return TRUE;
                    }
                    delete_object_cache(CDD(GfxBase)->planarbm_cache, GfxBase);
                }
                OOP_DisposeObject(CDD(GfxBase)->mdisplay.display_obj);
            }
            OOP_DisposeObject(CDD(GfxBase)->mdisplay.display_gfxhidd);
        }
    }

    D(bug("[graphics.library/driver] %s: init failed\n", __func__));
    return FALSE;
}

void driver_Queue(struct gfxboot_entry *boote, struct GfxBase * GfxBase)
{
    struct gfxboot_entry *last;
    for (last = (struct gfxboot_entry *)&PrivGBase(GfxBase)->boot_first; last->boot_next; last = last->boot_next) {}

    boote->boot_next = last->boot_next;
    last->boot_next = boote;
}

BOOL driver_Setup(struct gfxdriver_data *cfg, struct TagItem *attrs, BOOL force, struct GfxBase * GfxBase)
{
    if (!(cfg->drv_flags & DF_BootMode) || (force))
    {
        OOP_Object *gfxhidd = HW_AddDriver(PrivGBase(GfxBase)->GfxRoot, cfg->drv_class, attrs);

        if (gfxhidd)
        {
            OOP_Object *display;
            struct List *displayList;

            /* Attach system structures to the driver */
            OOP_GetAttr(gfxhidd, aHidd_Gfx_DisplayList, (IPTR *)&displayList);
            if (!displayList)
            {
                OOP_GetAttr(gfxhidd, aHidd_Gfx_DisplayDefault, (IPTR *)&display);
                if (display)
                    display_Register(display, cfg, force, GfxBase);
                else
                    D(bug("[graphics.library/driver] %s: Driver failed to expose any displays?\n", __func__));
            }
            else
            {
                ForeachNode(displayList, display)
                {
                    display_Register(display, cfg, force, GfxBase);
                }
            }
        }
        else
        {
            D(bug("[graphics.library/driver] %s: failed to instantiate driver\n", __func__));
            return FALSE;
        }
    }
    else
    {
        struct gfxboot_entry *boote = AllocMem(sizeof(struct gfxboot_entry), MEMF_ANY);
        if (boote)
        {
            boote->boot_next    = NULL;
            boote->boot_cfg     = cfg;
            boote->boot_attribs = attrs;
            driver_Queue(boote, GfxBase);
        }
        else
            return FALSE;
    }
    return TRUE;
}

/*
 * Iterate through HIDD_ViewPortData chains in the view and call the specified
 * function for every chain and every display.
 */
ULONG DoViewFunction(struct View *view, VIEW_FUNC fn, struct GfxBase *GfxBase)
{
    struct monitor_displaydata *mdd;
    ULONG rc = 0;

    ObtainSemaphoreShared(&CDD(GfxBase)->displaydb_sem);

    for (mdd = GFXPRIVATE_MONITORFIRST; mdd; mdd = (struct monitor_displaydata *)mdd->mdisplay.display_next)
    {
        struct HIDD_ViewPortData *vpd = NULL;

        if (view)
            vpd = display_FindViewPorts(mdd, view, GfxBase);

        rc = fn(mdd, vpd, view, GfxBase);

        if (rc)
            break;
    }

    ReleaseSemaphore(&CDD(GfxBase)->displaydb_sem);

    return rc;
}
