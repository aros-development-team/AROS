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
#include <graphics/driver.h>
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
        if (!baseGfx)
            return FALSE;
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

/*
 * Whether two dr_Size-terminated range arrays intersect anywhere.
 */
static BOOL driver_RangesOverlap(const struct DisplayRange *a,
                                 const struct DisplayRange *b)
{
    const struct DisplayRange *ap, *bp;

    for (ap = a; ap->dr_Size; ap++)
    {
        IPTR abase = (IPTR)ap->dr_Base;

        for (bp = b; bp->dr_Size; bp++)
        {
            IPTR bbase = (IPTR)bp->dr_Base;

            if ((abase < bbase + bp->dr_Size) && (bbase < abase + ap->dr_Size))
                return TRUE;
        }
    }
    return FALSE;
}

/*
 * Handover, first half: find a boot-mode display whose hardware the
 * caller is about to take over.
 *
 * This is what a native driver needs before it reprograms a card. On the
 * Titan the firmware's GOP framebuffer *is* the GPU's BAR1, so once
 * nouveau installs its own BAR1 page tables the boot console's redraws
 * stop landing in a framebuffer and start landing in whatever the new
 * owner put there - in that case the channels' USERD, which is why the
 * GPU stormed GPPTR errors while the GPU side was working perfectly.
 *
 * Matching is on CPU address ranges, so a boot driver serving a second,
 * unrelated card survives - that is the whole point of doing this by
 * range rather than shutting down every boot driver in the system.
 *
 * A boot driver that declared no DDRV_HWRanges is treated as a match:
 * historically DDRV_BootMode promised it would be taken down by the next
 * driver in, and a driver that never described its hardware must keep
 * getting that. Being wrong in this direction only costs a display;
 * being wrong in the other corrupts the new owner's state.
 *
 * Only valid from a driver's New(), where AddDisplayDriverA() holds
 * displaydb_sem on the caller's behalf.
 */
static APTR driver_FindBootDisplay(APTR ctx, const struct DisplayRange *ranges)
{
    struct GfxBase *GfxBase = (struct GfxBase *)ctx;
    struct gfxdisplay_data *mdd;

    for (mdd = (struct gfxdisplay_data *)CDD(GfxBase); mdd; mdd = mdd->display_next)
    {
        const struct DisplayRange *bootranges;

        if (!(mdd->display_flags & DF_BootMode))
            continue;
        if (mdd->display_flags & (DF_BootSurvive | DF_HandoverFail))
            continue;

        bootranges = mdd->display_cfg ? mdd->display_cfg->drv_ranges : NULL;

        if (!ranges || !bootranges ||
            driver_RangesOverlap(ranges, bootranges))
        {
            D(bug("[graphics.library/driver] %s: boot display 0x%p (ID 0x%08lX) shares the hardware\n",
                  __func__, mdd, mdd->display_idbase));
            return mdd;
        }

        D(bug("[graphics.library/driver] %s: boot display 0x%p (ID 0x%08lX) is on other hardware, left alone\n",
              __func__, mdd, mdd->display_idbase));
    }

    return NULL;
}

/*
 * Handover, second half: take the display found above out of the system
 * for good - unlinked from the display database, Intuition notified, its
 * modes no longer enumerated, its driver object disposed.
 *
 * Refuses, changing nothing, while anything is still displayed on it.
 * Migrating live screens to the incoming driver is a larger change and is
 * deliberately not attempted here; the caller finds out through the
 * return value, and the display is flagged so a find/expunge loop moves
 * past it instead of being handed it forever.
 */
static BOOL driver_ExpungeBootDisplay(APTR ctx, APTR handle)
{
    struct GfxBase *GfxBase = (struct GfxBase *)ctx;
    struct monitor_displaydata *mdd = (struct monitor_displaydata *)handle;

    if (!mdd)
        return FALSE;

    if (mdd->display || mdd->frontbm)
    {
        D(bug("[graphics.library/driver] %s: boot display 0x%p still in use, refusing\n",
              __func__, mdd));
        mdd->mdisplay.display_flags |= DF_HandoverFail;
        CDD(GfxBase)->handover_refused = TRUE;
        return FALSE;
    }

    D(bug("[graphics.library/driver] %s: expunging boot display 0x%p (ID 0x%08lX)\n",
          __func__, mdd, mdd->mdisplay.display_idbase));

    display_Expunge(mdd, GfxBase);
    return TRUE;
}


/*
 * Replay queued boot-mode drivers (registered with DDRV_BootMode) until one
 * of them provides the default monitor. driver_Setup() queues such drivers
 * instead of bringing them up, so a system whose only driver is a boot-mode
 * one (e.g. hosted headlessgfx) has an empty display database until this
 * runs. Entries that fail to instantiate are kept on the queue for a later
 * attempt, but each entry is tried at most once per call so a failing
 * driver cannot loop forever.
 */
void driver_ReplayBootQueue(struct GfxBase *GfxBase)
{
    struct gfxboot_entry *firstfailed = NULL;

    ObtainSemaphore(&CDD(GfxBase)->displaydb_sem);
    while (!GfxBase->default_monitor)
    {
        struct gfxboot_entry *boote = PrivGBase(GfxBase)->boot_first;

        if (!boote || boote == firstfailed)
            break;

        PrivGBase(GfxBase)->boot_first = boote->boot_next;

        if (driver_Setup(boote->boot_cfg, boote->boot_attribs, TRUE, GfxBase))
        {
            D(bug("[graphics.library/driver] %s: replayed queued boot driver 0x%p\n", __func__, boote->boot_cfg));
            FreeMem(boote, sizeof(struct gfxboot_entry));
            continue;
        }

        D(bug("[graphics.library/driver] %s: queued boot driver 0x%p failed to instantiate\n", __func__, boote->boot_cfg));
        boote->boot_next = NULL;
        driver_Queue(boote, GfxBase);
        if (!firstfailed)
            firstfailed = boote;
    }
    ReleaseSemaphore(&CDD(GfxBase)->displaydb_sem);
}

BOOL driver_Setup(struct gfxdriver_data *cfg, struct TagItem *attrs, BOOL force, struct GfxBase * GfxBase)
{
    if (!(cfg->drv_flags & DF_BootMode) || (force))
    {
        /*
         * A driver taking over real hardware is offered the handover
         * interface, so it can retire any boot-mode driver still writing
         * to that hardware before it reprograms it (see graphics/driver.h).
         *
         * The interface lives on this stack frame on purpose: it is only
         * usable from the driver's New(), which runs inside HW_AddDriver()
         * below, and stashing it for later is exactly what a driver must
         * not do.
         */
        struct DisplayHandover handover =
        {
            GfxBase,
            driver_FindBootDisplay,
            driver_ExpungeBootDisplay
        };
        struct TagItem handovertags[] =
        {
            { DDRVA_Handover, (IPTR)&handover },
            { TAG_MORE,       (IPTR)attrs     }
        };
        OOP_Object *gfxhidd;
        BOOL offer = !(cfg->drv_flags & (DF_BootMode | DF_KeepBoot));

        if (offer)
        {
            struct gfxdisplay_data *scan;

            /*
             * DF_HandoverFail only exists to stop one driver's
             * find/expunge loop being handed the same unreleasable
             * display forever. It must not outlive that loop: whatever
             * was in use may well have gone by the time the next driver
             * tries, and that one has to be offered the display again.
             */
            for (scan = (struct gfxdisplay_data *)CDD(GfxBase); scan; scan = scan->display_next)
                scan->display_flags &= ~DF_HandoverFail;
        }

        gfxhidd = HW_AddDriver(PrivGBase(GfxBase)->GfxRoot, cfg->drv_class,
                               offer ? handovertags : attrs);

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
