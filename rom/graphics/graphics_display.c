/*
    Copyright (C) 2016-2026, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
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

#include <utility/tagitem.h>
#include <aros/asmcall.h>

#include <intuition/intuition.h>

#include <hidd/hidd.h>

#include <cybergraphx/cybergraphics.h>

#include <stdio.h>
#include <string.h>

#include "graphics_intern.h"
#include "graphics_display.h"
#include "gfxfuncsupport.h"
#include "dispinfo.h"

/*
 * Set up graphics display driver.
 * Creates necessary OS structures around it.
 *
 * gfxhidd - newly created driver object
 * result  - master driver structure
 */

struct gfxdisplay_data *display_Setup(OOP_Object *displayhidd, struct GfxBase *GfxBase)
{
    ULONG cnt = 0;
    IPTR softcursor;
    IPTR fbtype = vHidd_FrameBuffer_Direct; /* We default to direct for historical reasons */
    BOOL ok = TRUE;
    ULONG i;
    HIDDT_ModeID *modes, *m;
    struct monitor_displaydata *mdd;
    OOP_Object *dmenum;

    D(bug("[graphics.library/display] %s: '%s' @ 0x%p\n", __func__, OOP_OCLASS(displayhidd)->ClassNode.ln_Name, displayhidd));

    OOP_GetAttr(displayhidd, aHidd_Display_DMEnumerator, (IPTR *)&dmenum);
    D(bug("[graphics.library/display] %s: dmenum @ 0x%p\n", __func__, dmenum));
    if (!dmenum)
        return NULL;

    modes = HIDD_DMEnum_QueryModeIDs(dmenum, NULL);
    D(bug("[graphics.library/display] %s: modes @ 0x%p\n", __func__, modes));
    if (!modes)
        return NULL;

    /* Count number of display modes */
    for (m = modes; *m != vHidd_ModeID_Invalid; m ++)
	cnt++;

    mdd = AllocVec(sizeof(struct monitor_displaydata) + cnt * sizeof(struct DisplayInfoHandle), MEMF_PUBLIC|MEMF_CLEAR);
    D(bug("[graphics.library/display] %s: Allocated driverdata at 0x%p for %d modes\n", __func__, mdd, cnt));
    if (!mdd)
    {
        HIDD_DMEnum_ReleaseModeIDs(dmenum, modes);
        return NULL;
    }

    OOP_GetAttr(displayhidd, aHidd_Display_GfxHidd, (IPTR *)&mdd->mdisplay.display_gfxhidd);
#ifndef FORCE_SOFTWARE_SPRITE
    OOP_GetAttr(displayhidd, aHidd_Display_SoftCursor, &softcursor);
    if (softcursor)
#endif
    {
        mdd->mdisplay.display_flags |= DF_SoftCursor;
    }

    mdd->mdisplay.display_dmenum = dmenum;

    {
        IPTR pri = 0;
        OOP_GetAttr(dmenum, aHidd_DMEnum_Priority, &pri);
        mdd->mdisplay.display_pri = (WORD)pri;
    }
    D(bug("[graphics.library/display] %s: GfxHidd at 0x%p\n", __func__, mdd->mdisplay.display_gfxhidd));

    /* Fill in ModeID database in the driverdata. */
    for (i = 0; i <= cnt; i++)
    {
        mdd->modes[i].id  = modes[i];
	mdd->modes[i].drv = (struct gfxdisplay_data *)mdd;
    }

    HIDD_DMEnum_ReleaseModeIDs(mdd->mdisplay.display_dmenum, modes);
    mdd->mdisplay.display_obj = displayhidd;

    /* Damage notification runs after every render operation: resolve it once */
    mdd->mdisplay.display_UpdateRect = OOP_GetMethod(displayhidd, HiddDisplayBase + moHidd_Display_UpdateRect,
                                                     &mdd->mdisplay.display_UpdateRect_cl);

    /* Query properties of our driver */
    OOP_GetAttr(mdd->mdisplay.display_gfxhidd, aHidd_Gfx_FrameBufferType, &fbtype);

    if (ok)
    {
        /* Instantiate framebuffer if needed. */
	D(bug("[graphics.library/display] %s: Ok, framebuffer type %ld\n", __func__, fbtype));
        switch (fbtype)
        {
        case vHidd_FrameBuffer_Direct:
            mdd->mdisplay.display_flags |= DF_DirectFB;

        case vHidd_FrameBuffer_Mirrored:
	    mdd->framebuffer = display_FBCreate(mdd, GfxBase);
            break;
	}

	if ((fbtype == vHidd_FrameBuffer_None) || mdd->framebuffer)
	{
	    D(
                if (mdd->framebuffer)
                    bug("[graphics.library/display] %s: FRAMEBUFFER OK: %p\n", __func__, mdd->framebuffer);
            )

	    /*
	     * Whether screens compose on this display (dragging, alpha) is the
	     * display's business: its driver either composes in hardware
	     * (ModeProperties reports COMPF_* flags) or its compositor does.
	     */
	    {
	        IPTR caps = 0;

	        OOP_GetAttr(displayhidd, aHidd_Display_CompositionCapabilities, &caps);
	        if (caps)
	            mdd->mdisplay.display_flags |= DF_Composes;
	    }

	    return (struct gfxdisplay_data *)mdd;
	}

	if (mdd->framebuffer)
	    OOP_DisposeObject(mdd->framebuffer);
    }

    FreeVec(mdd);

    return NULL;
}

/*
 * Completely remove a display from the OS: unlinked from the display
 * chain, Intuition told to drop its monitor object, mode database freed,
 * and - once the last of its displays has gone - the driver object it
 * belongs to removed from the HW root as well.
 *
 * mdd - Display structure to remove.
 *
 * Note that removing a display is an unsafe operation. You must be sure
 * that nothing is displayed on it and that no bitmaps of its driver exist
 * at the moment; only the caller can know that.
 *
 * Caller holds displaydb_sem.
 */

void display_Expunge(struct monitor_displaydata *mdd, struct GfxBase *GfxBase)
{
    struct gfxdisplay_data *pred, *scan;
    OOP_Object *gfxhidd = mdd->mdisplay.display_gfxhidd;
    BOOL lastdisplay = TRUE;

    /*
     * Unlink first: everything below can re-enter graphics.library, and
     * from here on nothing may find this display any more.
     */
    for (pred = (struct gfxdisplay_data *)CDD(GfxBase); pred; pred = pred->display_next)
    {
        if (pred->display_next == (struct gfxdisplay_data *)mdd)
        {
            pred->display_next = mdd->mdisplay.display_next;
            break;
        }
    }
    mdd->mdisplay.display_next = NULL;

    /* Notify Intuition */
    if (CDD(GfxBase)->DriverNotify)
	CDD(GfxBase)->DriverNotify(mdd->mdisplay.display_userdata, FALSE, CDD(GfxBase)->mdisplay.display_userdata);

    /* Dispose associated stuff (the compositor goes with the display object) */
    if (mdd->framebuffer)
        OOP_DisposeObject(mdd->framebuffer);

    /* Dispose driver object. This will take care about syncs etc */
    OOP_DisposeObject(mdd->mdisplay.display_obj);

    /*
     * A driver can expose several displays. Only when its last one has
     * been removed does the driver object itself go - which is what
     * actually stops a boot-mode driver touching the hardware.
     */
    for (scan = (struct gfxdisplay_data *)CDD(GfxBase); scan; scan = scan->display_next)
    {
        if (scan->display_gfxhidd == gfxhidd)
        {
            lastdisplay = FALSE;
            break;
        }
    }

    if (lastdisplay && gfxhidd && (gfxhidd != CDD(GfxBase)->mdisplay.display_gfxhidd))
    {
        D(bug("[graphics.library/display] %s: removing driver object 0x%p\n", __func__, gfxhidd));
        HW_RemoveDriver(PrivGBase(GfxBase)->GfxRoot, gfxhidd);
    }

    FreeVec(mdd);

    /*
     * The default monitor may have been provided by the display just
     * removed, and its modes may have been the ones that pushed the next
     * free ID up. Rebuild both from what is left.
     */
    GfxBase->default_monitor = NULL;
    CDD(GfxBase)->default_monitor_pri = 0;

    for (scan = CDD(GfxBase)->mdisplay.display_next; scan; scan = scan->display_next)
    {
        OOP_Object *sync;

        if (!(scan->display_flags & DF_Enabled))
            continue;

        sync = HIDD_DMEnum_GetSync(scan->display_dmenum, 0);
        if (!sync)
            continue;

        if ((!GfxBase->default_monitor) ||
            (scan->display_pri > CDD(GfxBase)->default_monitor_pri))
        {
            OOP_GetAttr(sync, aHidd_Sync_MonitorSpec, (IPTR *)&GfxBase->default_monitor);
            CDD(GfxBase)->default_monitor_pri = scan->display_pri;
        }
    }

    D(bug("[graphics.library/display] %s: default monitor is now 0x%p\n", __func__, GfxBase->default_monitor));
}

void display_Queue(struct gfxdisplay_data *mdd, struct gfxdisplay_data *last, struct GfxBase *GfxBase)
{
    for (; last->display_next; last = last->display_next)
    {
        D(bug("[graphics.library/display] %s: Current 0x%p, next 0x%p, ID 0x%08lX\n", __func__, last, last->display_next, last->display_next->display_idbase));
        /* Higher-priority displays enumerate first; equal priorities
           keep the historical monitor ID ordering */
        if (mdd->display_pri > last->display_next->display_pri)
            break;
        if ((mdd->display_pri == last->display_next->display_pri) &&
            (mdd->display_idbase < last->display_next->display_idbase))
            break;
    }

    D(
        if (last != (struct gfxdisplay_data *)&PrivGBase(GfxBase)->boot_first)
        {
            bug("[graphics.library/display] %s: Inserting after 0x%p\n", __func__, last);
        }
        else
        {
            bug("[graphics.library/display] %s: First entry..\n", __func__);
        }
    )
    mdd->display_next = last->display_next;
    last->display_next = mdd;
}

void display_Enable(struct gfxdisplay_data *mdd, struct GfxBase *GfxBase)
{
    if (CDD(GfxBase)->DriverNotify)
    {
        D(bug("[graphics.library/display] %s: Calling Notify Hook (TRUE, 0x%p)\n", __func__, CDD(GfxBase)->mdisplay.display_userdata));
        mdd->display_userdata = CDD(GfxBase)->DriverNotify(mdd, TRUE, CDD(GfxBase)->mdisplay.display_userdata);
    }

    /* Remember next available ID */
    if (mdd->display_cfg->drv_idnext > GFXPRIVATE_MODELAST)
        GFXPRIVATE_MODELAST = mdd->display_cfg->drv_idnext;

    /* Return the assigned ID if the caller asked to do so */
    if (mdd->display_cfg->drv_idstore)
        *mdd->display_cfg->drv_idstore = mdd->display_cfg->drv_idbase;

    if ((!GfxBase->default_monitor) ||
        (mdd->display_pri > CDD(GfxBase)->default_monitor_pri))
    {
        D(bug("[graphics.library/display] %s: Setting default_monitor\n", __func__));
        OOP_Object *sync = HIDD_DMEnum_GetSync(mdd->display_dmenum, 0);
        /* A display registering no modes of its own has no sync 0 - it cannot
           provide the default monitor */
        if (sync)
        {
            OOP_GetAttr(sync, aHidd_Sync_MonitorSpec, (IPTR *)&GfxBase->default_monitor);
            CDD(GfxBase)->default_monitor_pri = mdd->display_pri;
        }
    }
    mdd->display_flags |= DF_Enabled;
}

void display_Register(OOP_Object *display, struct gfxdriver_data *cfg, BOOL force, struct GfxBase *GfxBase)
{
    struct gfxdisplay_data *mdd = display_Setup(display, GfxBase);


    if (mdd)
    {
        mdd->display_idbase     = cfg->drv_idbase;
        mdd->display_mask       = cfg->drv_idmask;
        /*
         * Merge (do not overwrite) the driver-config flags into the flags that
         * display_Setup() already derived from the hardware (DF_DirectFB,
         * DF_SoftComposit, DF_SoftCursor). Overwriting them here would discard the
         * composition flag.
         */
        mdd->display_flags     |= cfg->drv_flags;
        mdd->display_cfg        = cfg;

        /*
         * The display (and its compositor, which allocates the bitmap it
         * composes into on NoFrameBuffer drivers) needs the public ID base
         * to build complete DisplayIDs.
         */
        OOP_SetAttrsTags(display, aHidd_Display_DisplayID, cfg->drv_idbase, TAG_DONE);

        if ((mdd->display_flags & DF_BootMode) && (!force))
        {
            D(bug("[graphics.library/display] %s: Queueing BootMode driver 0x%p, ID 0x%08lX\n", __func__, mdd, mdd->display_idbase));
            display_Queue(mdd, (struct gfxdisplay_data *)&PrivGBase(GfxBase)->boot_first, GfxBase);
        }
        else
        {
            /* Insert the driverdata into chain, sorted by ID */
            D(bug("[graphics.library/display] %s: Inserting driver 0x%p, ID 0x%08lX\n", __func__, mdd, mdd->display_idbase));
            display_Queue(mdd, (struct gfxdisplay_data *)CDD(GfxBase), GfxBase);
            display_Enable(mdd, GfxBase);
        }
    }
}

/* Called after DOS is up & running */
OOP_Object *display_FBCreate(struct monitor_displaydata *mdd, struct GfxBase *GfxBase)
{
    struct TagItem fbtags[] = {
    	{ aHidd_BitMap_FrameBuffer,	TRUE	},
	{ aHidd_BitMap_ModeID,		0	},
	{ TAG_DONE, 0 }
    };

    HIDDT_ModeID hiddmode;
    OOP_Object *fb = NULL;

    /* Get the highest available resolution at the best possible depth */
    hiddmode = get_best_resolution_and_depth(mdd, GfxBase);
    if (vHidd_ModeID_Invalid == hiddmode) {
    	D(bug("[graphics.library/display] %s: COULD NOT GET HIDD MODEID !!!\n", __func__));
    } else {
    	/* Create the framebuffer object */
	fbtags[1].ti_Data = hiddmode;
	fb = HIDD_Display_CreateObject(mdd->mdisplay.display_obj, PrivGBase(GfxBase)->basebm, fbtags);
    }

    return fb;
}

/*
 * In-Place framebuffer install/uninstall routines.
 *
 * The idea behind: we gave some bitmap to display, the driver *HAS COPIED* it
 * into the framebuffer. After this we must use a framebuffer bitmap instead of
 * original one for all rendering operations.
 * When another bitmap is displayed, the framebuffer contents will be put back
 * into the original bitmap, and we will swap it back.
 *
 * This swap actually happens only of DF_DirectFB is set for the driver.
 *
 * FIXME: THIS IS NOT THREADSAFE
 * To make this threadsafe we have to lock all gfx access in all the rendering
 * calls.
 */
void display_FBInstall(struct monitor_displaydata *mdd, struct GfxBase *GfxBase)
{
    if ((mdd->mdisplay.display_flags & DF_DirectFB) && mdd->frontbm)
    {
	struct BitMap *bitmap = mdd->frontbm;
    	OOP_Object *pf;

    	/* Backup original data */
    	mdd->bm_bak     = HIDD_BM_OBJ(bitmap);
    	mdd->colmod_bak = HIDD_BM_COLMOD(bitmap);
    	mdd->colmap_bak = HIDD_BM_COLMAP(bitmap);

    	/* Insert the framebuffer in its place */
    	OOP_GetAttr(mdd->framebuffer, aHidd_BitMap_PixFmt, (IPTR *)&pf);

    	HIDD_BM_OBJ(bitmap)    = mdd->framebuffer;
    	HIDD_BM_COLMOD(bitmap) = OOP_GET(pf, aHidd_PixFmt_ColorModel);
    	HIDD_BM_COLMAP(bitmap) = (OOP_Object *)OOP_GET(mdd->framebuffer, aHidd_BitMap_ColorMap);

    	DEBUG_LOADVIEW(
            bug("[graphics.library/display] %s: Installed framebuffer\n", __func__);
            bug("[graphics.library/display] %s:       BitMap 0x%p, object 0x%p\n", __func__, mdd->frontbm, mdd->bm_bak);
        )
    }
}

void display_FBUninstall(struct monitor_displaydata *mdd, struct GfxBase *GfxBase)
{
    if (mdd->bm_bak)
    {
        /* Put back the old values into the old bitmap */
        struct BitMap *oldbm = mdd->frontbm;

    	DEBUG_LOADVIEW(
            bug("[graphics.library/display] %s: Uninstalling framebuffer\n", __func__);
            bug("[graphics.library/display] %s:       BitMap 0x%p, object 0x%p\n", __func__, mdd->frontbm, mdd->bm_bak);
        )
        HIDD_BM_OBJ(oldbm)    = mdd->bm_bak;
        HIDD_BM_COLMOD(oldbm) = mdd->colmod_bak;
        HIDD_BM_COLMAP(oldbm) = mdd->colmap_bak;

	/* No framebuffer installed */
        mdd->bm_bak = NULL;
    }
}

/* This is called for every ViewPorts chain during MrgCop() */
ULONG display_PrepareViewPorts(struct monitor_displaydata *mdd, struct HIDD_ViewPortData *vpd, struct View *v, struct GfxBase *GfxBase)
{
    /* Do not bother the driver if there's nothing to prepare. Can be changed if needed. */
    return vpd ? HIDD_Display_InitViewPorts(mdd->mdisplay.display_obj, vpd, v) : MCOP_OK;
}

/*
 * This is called for every ViewPorts chain during LoadView()
 * LoadView() actually has no rights for errors because error condition
 * in it will screw up the display, likely with no chance to recover.
 * Because of this we always return zero here.
 */
ULONG display_LoadViewPorts(struct monitor_displaydata *mdd, struct HIDD_ViewPortData *vpd, struct View *v, struct GfxBase *GfxBase)
{
    struct BitMap *bitmap = vpd ? vpd->vpe->ViewPort->RasInfo->BitMap : NULL;
    OOP_Object *fb;
    BOOL compositing = FALSE;

    DEBUG_LOADVIEW(bug("[graphics.library/display] %s: Showing ViewPortData 0x%p, BitMap object 0x%p\n", __func__, vpd, vpd ? vpd->Bitmap : NULL));
    mdd->display = vpd;

    /* A driver composing in hardware takes the whole chain */
    if (HIDD_Display_ShowViewPorts(mdd->mdisplay.display_obj, vpd))
    {
        DEBUG_LOADVIEW(bug("[graphics.library/display] %s: ShowViewPorts() worked\n", __func__));
        mdd->frontbm = bitmap;
        return 0;
    }

    /*
     * Otherwise the display's compositor shows it: the frontmost bitmap
     * directly when it covers the display, a composed bitmap otherwise. It
     * handles all possible failures internally and falls back to showing
     * the frontmost bitmap alone.
     */
    fb = HIDD_Display_ComposeViewPorts(mdd->mdisplay.display_obj, vpd, &compositing);
    DEBUG_LOADVIEW(bug("[graphics.library/display] %s: Compositor returned 0x%p, active: %d\n", __func__, fb, compositing));

    /* Summary of possible responses (when no error happened):
       NoFrameBuffer = FALSE: bm = NULL  -> fb != NULL and bm != fb
       NoFrameBuffer = FALSE: bm != NULL -> fb != NULL and bm != fb
       NoFrameBuffer = TRUE : bm = NULL  -> fb == NULL and bm == fb
       NoFrameBuffer = TRUE : bm != NULL -> fb != NULL and bm == fb
    */
    if (fb)
    {
	/* The screen is not empty, 'fb' is on display now. */
        IPTR width, height;

	/* Uninstall the framebuffer from old frontmost BitMap (if present) */
	display_FBUninstall(mdd, GfxBase);

    	/*
     	 * We need to always remember our new frontmost bitmap, even if we do not work
     	 * with a framebuffer. This is needed for the check against showing the same
     	 * bitmap twice.
     	 */
    	mdd->frontbm = bitmap;

	/*
	 * Install the framebuffer into new bitmap. Only if composition is inactive.
	 * If it is active, our BitMap is mirrored, not replaced.
	 */
	if (!compositing)
	    display_FBInstall(mdd, GfxBase);

        /* Tell the driver to refresh the screen */
        OOP_GetAttr(fb, aHidd_BitMap_Width, &width);
        OOP_GetAttr(fb, aHidd_BitMap_Height, &height);
        DEBUG_LOADVIEW(bug("[graphics.library/display] %s: Updating display, new size: %d x %d\n", __func__, width, height));

        HIDD_BM_UpdateRect(fb, 0, 0, width, height);
    }
    else
    {
    	/*
    	 * Screen is empty, simply reset the current bitmap.
    	 * Only framebuffer-less driver can return NULL. So we don't need
    	 * to call display_FBUninstall() here.
    	 */
        mdd->frontbm = NULL;
    }

    return 0;
}

/*
 * Custom screen bitmaps (OpenScreen() with SA_BitMap): a bitmap that is not
 * displayable itself can still be shown when the display composes screens.
 * A display composing in hardware decides for itself through ShowViewPorts();
 * the compositor shows any HIDD bitmap it can read. Classic planar bitmaps in
 * Chip RAM are wrapped by MakeVPort() on displays that scan them out directly.
 */
static BOOL display_CanDisplayExternalPlanar(struct BitMap *bitmap, struct gfxdisplay_data *display, struct GfxBase *GfxBase)
{
    UBYTE plane;

    if (!(display->display_flags & DF_ExternalPlanar) || !bitmap ||
        IS_HIDD_BM(bitmap) || !bitmap->Rows || !bitmap->BytesPerRow ||
        !bitmap->Depth || bitmap->Depth > 8)
        return FALSE;

    for (plane = 0; plane < bitmap->Depth; plane++)
    {
        if (!bitmap->Planes[plane] ||
            !(TypeOfMem(bitmap->Planes[plane]) & MEMF_CHIP))
            return FALSE;
    }

    return TRUE;
}

/* The display's compositor (not the driver) composes screens on this display */
BOOL display_SoftComposes(struct gfxdisplay_data *display, HIDDT_ModeID hiddmode, struct GfxBase *GfxBase)
{
    struct HIDD_ModeProperties props = {0};

    if (!(display->display_flags & DF_Composes))
        return FALSE;

    HIDD_Display_ModeProperties(display->display_obj, hiddmode, &props, sizeof(props));
    return props.CompositionFlags == 0;
}

BOOL display_IsBMCompositable(struct BitMap *bitmap, DisplayInfoHandle handle, struct GfxBase *GfxBase)
{
    if (display_SoftComposes(DIH(handle)->drv, DIH(handle)->id, GfxBase))
        return IS_HIDD_BM(bitmap);

    return display_CanDisplayExternalPlanar(bitmap, DIH(handle)->drv, GfxBase);
}

BOOL display_SetBMCompositable(struct BitMap *bitmap, DisplayInfoHandle handle, struct GfxBase *GfxBase)
{
    if (display_SoftComposes(DIH(handle)->drv, DIH(handle)->id, GfxBase))
    {
        if (!IS_HIDD_BM(bitmap))
            return FALSE;

        if (!OOP_GET(HIDD_BM_OBJ(bitmap), aHidd_BitMap_Displayable))
        {
            struct TagItem composittags[] =
            {
                {aHidd_BitMap_Compositable, TRUE},
                {TAG_DONE                 , 0   }
            };

            OOP_SetAttrs(HIDD_BM_OBJ(bitmap), composittags);
        }

        /*
         * A bitmap allocated with an explicit pixel format (BMF_SPECIALFMT,
         * the way to get an ARGB screen bitmap) carries no mode ID: AllocBitMap()
         * drops it in favour of the format. It is about to be shown through
         * this display's compositor in the mode OpenScreen() picked, so give
         * it that mode now. MakeVPort() checks the bitmap's mode and driver
         * against the viewport's colormap and would otherwise refuse it.
         */
        if (HIDD_BM_HIDDMODE(bitmap) == vHidd_ModeID_Invalid)
        {
            HIDD_BM_HIDDMODE(bitmap) = DIH(handle)->id;
            HIDD_BM_DRVDATA(bitmap)  = (struct monitor_driverdata *)DIH(handle)->drv;
        }

        return TRUE;
    }
    /* MakeVPort wraps classic planar BitMaps without copying their planes. */
    return display_CanDisplayExternalPlanar(bitmap, DIH(handle)->drv, GfxBase);
}

/* Find the first visible ViewPortData for the specified monitor in the View */
struct HIDD_ViewPortData *display_FindViewPorts(struct monitor_displaydata *mdd, struct View *view, struct GfxBase *GfxBase)
{
    struct ViewPort *vp;

    for (vp = view->ViewPort; vp; vp = vp->Next)
    {
	if (!(vp->Modes & VP_HIDE))
	{
	    struct ViewPortExtra *vpe = (struct ViewPortExtra *)GfxLookUp(vp);
	    if (VPE_DRIVER(vpe) == (APTR)mdd)
	    	return VPE_DATA(vpe);
	}
    }
    return NULL;
}
