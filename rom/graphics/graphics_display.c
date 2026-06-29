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

#include <cybergraphx/cybergraphics.h>

#include <stdio.h>
#include <string.h>

#include "graphics_intern.h"
#include "graphics_display.h"
#include "graphics_compositor.h"
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
    UWORD compose = 0;
    BOOL can_compose = FALSE;
    BOOL ok = TRUE;
    ULONG i;
    HIDDT_ModeID *modes, *m;
    struct monitor_displaydata *mdd;
    struct HIDD_ModeProperties props = {0};
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
    D(bug("[graphics.library/display] %s: GfxHidd at 0x%p\n", __func__, mdd->mdisplay.display_gfxhidd));

    /*
     * 1. Fill in ModeID database in the driverdata.
     * 2. Check if at least one mode supports composition.
     * 3. Check if the driver has at least one non-palettized mode.
     * (2) and (3) are needed to determine if we need/can use software screen
     * composition with this driver.
     */
    for (i = 0; i <= cnt; i++)
    {
        mdd->modes[i].id  = modes[i];
	mdd->modes[i].drv = (struct gfxdisplay_data *)mdd;

	/* Watch out! The last record in the array is terminator (vHidd_ModeID_Invalid) */
	if ((i < cnt) && (!compose))
	{
	    HIDD_Display_ModeProperties(displayhidd, modes[i], &props, sizeof(props));
	    compose |= props.CompositionFlags;

	    if (!can_compose)
	    {
		OOP_Object *sync, *pf;
		IPTR colmod;

		HIDD_DMEnum_GetMode(mdd->mdisplay.display_dmenum, modes[i], &sync, &pf);
                if (pf)
                {
                    OOP_GetAttr(pf, aHidd_PixFmt_ColorModel, &colmod);

                    if (colmod == vHidd_ColorModel_TrueColor)
                    {
                        /*
                         * At the moment software compositor supports only truecolor screens.
                         * There also definitions for DirectColor, gray, etc, but there is
                         * no such hardware supported by now.
                         */
                        can_compose = TRUE;
                    }
                }
	    }
	}
    }

    HIDD_DMEnum_ReleaseModeIDs(mdd->mdisplay.display_dmenum, modes);
    mdd->mdisplay.display_obj = displayhidd;

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

	    if ((!compose) && can_compose)
	    {
		D(bug("[graphics.library/display] %s: Software screen composition required\n", __func__));

		mdd->mdisplay.display_flags |= DF_SoftComposit;
		/*
		 * NB: the compositor is instantiated later, from display_Register(),
		 * once display_idbase has been assigned. The compositor needs the id
		 * base in order to build a valid DisplayID for the bitmap it allocates
		 * to composite into on NoFrameBuffer drivers; setting it up here (before
		 * the id base is known) would bake in an invalid (zero) DisplayID.
		 */
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
 * Completely remove a driver from the OS.
 *
 * mdd - Driver structure to remove.
 *
 * Note that removing a driver is very unsafe operation. You must be
 * sure that no bitmaps of this driver exist at the moment. Perhaps
 * something should be done with it.
 *
 */

void display_Expunge(struct monitor_displaydata *mdd, struct GfxBase *GfxBase)
{
    /* Notify Intuition */
    if (CDD(GfxBase)->DriverNotify)
	CDD(GfxBase)->DriverNotify(mdd->mdisplay.display_userdata, FALSE, CDD(GfxBase)->mdisplay.display_userdata);

    /* Dispose associated stuff */
    if (mdd->mdisplay.display_compositor)
        OOP_DisposeObject(mdd->mdisplay.display_compositor);
    if (mdd->framebuffer)
        OOP_DisposeObject(mdd->framebuffer);

    /* Dispose driver object. This will take care about syncs etc */
    OOP_DisposeObject(mdd->mdisplay.display_obj);

    FreeVec(mdd);
}

void display_Queue(struct gfxdisplay_data *mdd, struct gfxdisplay_data *last, struct GfxBase *GfxBase)
{
    for (; last->display_next; last = last->display_next)
    {
        D(bug("[graphics.library/display] %s: Current 0x%p, next 0x%p, ID 0x%08lX\n", __func__, last, last->display_next, last->display_next->display_idbase));
        if (mdd->display_idbase < last->display_next->display_idbase)
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

    if (!GfxBase->default_monitor)
    {
        D(bug("[graphics.library/display] %s: Setting default_monitor\n", __func__));
        OOP_Object *sync = HIDD_DMEnum_GetSync(mdd->display_dmenum, 0);
        D(bug("[graphics.library/display] %s: sync @ 0x%p\n", __func__, sync));
        OOP_GetAttr(sync, aHidd_Sync_MonitorSpec, (IPTR *)&GfxBase->default_monitor);
    }
    mdd->display_flags |= DF_Enabled;
}

void display_Register(OOP_Object *display, struct gfxdriver_data *cfg, BOOL force, struct GfxBase *GfxBase)
{
    struct gfxdisplay_data *mdd = display_Setup(display, GfxBase);

    D(bug("[graphics.library/display] %s: internal monitor display data 0x%p\n", __func__, mdd));

    if (mdd)
    {
        mdd->display_idbase     = cfg->drv_idbase;
        mdd->display_mask       = cfg->drv_idmask;
        /*
         * Merge (do not overwrite) the driver-config flags into the flags that
         * display_Setup() already derived from the hardware (DF_DirectFB,
         * DF_SoftComposit, DF_SoftCursor). Overwriting them here would discard the
         * software-composition request and break the compositor retry path in
         * compositor_Install() when the Compositor class registers after the display.
         */
        mdd->display_flags     |= cfg->drv_flags;
        mdd->display_cfg        = cfg;

        /*
         * Now that display_idbase is known, instantiate the software compositor
         * if this display needs it and the Compositor class is already
         * registered. (When the class registers later, compositor_Install()
         * performs this same setup; by then the id base is already assigned.)
         */
        if ((mdd->display_flags & DF_SoftComposit) && CDD(GfxBase)->compositorClass &&
            !mdd->display_compositor)
        {
            compositor_Setup((struct monitor_displaydata *)mdd, GfxBase);
        }

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
    struct BitMap *bitmap;
    OOP_Object *bm, *fb;
    BOOL compositing = FALSE;

    DEBUG_LOADVIEW(bug("[graphics.library/display] %s: Showing ViewPortData 0x%p, BitMap object 0x%p\n", __func__, vpd, vpd ? vpd->Bitmap : NULL));
    mdd->display = vpd;

    /* First try the new method */
    if (HIDD_Display_ShowViewPorts(mdd->mdisplay.display_obj, vpd))
    {
        DEBUG_LOADVIEW(bug("[graphics.library/display] %s: ShowViewPorts() worked\n", __func__));
        return 0;
    }

    /*
     * ShowViewPorts not supported.
     * Perhaps we can use software screen compositor. But for proper operation
     * we need to figure out our frontmost bitmap.
     */
    if (vpd)
    {
        bitmap = vpd->vpe->ViewPort->RasInfo->BitMap;
        bm     = vpd->Bitmap;
    }
    else
    {
        bitmap = NULL;
        bm     = NULL;
    }
    DEBUG_LOADVIEW(bug("[graphics.library/display] %s: Old bitmap 0x%p, New bitmap 0x%p, object 0x%p\n", __func__, mdd->frontbm, bitmap, bm));

    if (mdd->mdisplay.display_compositor)
    {
    	/*
    	 * Compositor present. Give ViewPorts chain to it.
    	 * For improved visual appearance it will call Show itself and return its result.
    	 * The compositor is expected to handle all possible failures internally. If some
    	 * internal error happens, it must fail back to single HIDD_Gfx_Show().
    	 */
    	fb = compositor_LoadViewPorts(mdd->mdisplay.display_compositor, vpd, &compositing, GfxBase);
    	DEBUG_LOADVIEW(bug("[graphics.library/display] %s: Compositor returned 0x%p, active: %d\n", __func__, fb, compositing));
    }
    else
    {
    	/*
     	 * First check if the bitmap is already displayed. If so, do nothing (because
     	 * displaying the same bitmap twice may cause some problems)
     	 */
    	if (mdd->frontbm == bitmap)
            return 0;

    	fb = HIDD_Display_Show(mdd->mdisplay.display_obj, bm, fHidd_Display_Show_CopyBack);
    	DEBUG_LOADVIEW(bug("[graphics.library/display] %s:] Show() returned 0x%p\n", __func__, fb));
    }

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
	 * Install the framebuffer into new bitmap. Only if software composition is inactive.
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
