/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Driver for using gfxhidd for gfx output
    Lang: english
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

#include <intuition/intuition.h>

#include <hidd/compositor.h>
#include <hidd/graphics.h>

#include <cybergraphx/cybergraphics.h>

#include <stdio.h>
#include <string.h>

#include "graphics_intern.h"
#include "compositor_driver.h"
#include "fakegfxhidd.h"
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

struct ETextFont
{
    struct TextFont	etf_Font;
};

/* *********************** RastPort extra data handling *********************** */

struct gfx_driverdata *AllocDriverData(struct RastPort *rp, BOOL alloc, struct GfxBase *GfxBase)
{
    struct gfx_driverdata *dd = ObtainDriverData(rp);

    if (alloc && !dd)
    {
    	dd = AllocVec(sizeof(struct gfx_driverdata), MEMF_CLEAR);
    	if (dd)
    	{
    	    rp->RP_Extra    = dd;
    	    dd->dd_RastPort = rp;
    	}
    }

    return dd;
}    

/* *********************** Display driver handling *********************** */

int driver_init(struct GfxBase * GfxBase)
{

    EnterFunc(bug("driver_init()\n"));

    /* Our underlying RTG subsystem core must be already up and running */
    if (!OpenLibrary("graphics.hidd", 0))
        return FALSE;

    /* Initialize the semaphores */
    InitSemaphore(&(PrivGBase(GfxBase)->blit_sema));
    
    /* Init the needed attrbases */

    __IHidd_BitMap  	= OOP_ObtainAttrBase(IID_Hidd_BitMap);
    __IHidd_GC      	= OOP_ObtainAttrBase(IID_Hidd_GC);
    __IHidd_Sync    	= OOP_ObtainAttrBase(IID_Hidd_Sync);
    __IHidd_PixFmt  	= OOP_ObtainAttrBase(IID_Hidd_PixFmt);
    __IHidd_PlanarBM 	= OOP_ObtainAttrBase(IID_Hidd_PlanarBM);
    __IHidd_Gfx     	= OOP_ObtainAttrBase(IID_Hidd_Gfx);
    __IHidd_FakeGfxHidd = OOP_ObtainAttrBase(IID_Hidd_FakeGfxHidd);

    if (__IHidd_BitMap   &&
        __IHidd_GC       &&
	__IHidd_Sync     &&
	__IHidd_PixFmt   &&
	__IHidd_PlanarBM &&
	__IHidd_Gfx      &&
	__IHidd_FakeGfxHidd)
    {
        CDD(GfxBase)->gcClass = OOP_FindClass(CLID_Hidd_GC);
    	if (!CDD(GfxBase)->gcClass)
    	    return FALSE;

	/* Init display mode database */
	InitSemaphore(&CDD(GfxBase)->displaydb_sem);
	CDD(GfxBase)->invalid_id = INVALID_ID;
	CDD(GfxBase)->last_id = AROS_RTG_MONITOR_ID;

	/* Init memory driver */
	CDD(GfxBase)->memorygfx = OOP_NewObject(NULL, CLID_Hidd_Gfx, NULL);
	DEBUG_INIT(bug("[driver_init] Memory driver object 0x%p\n", CDD(GfxBase)->memorygfx));

	if (CDD(GfxBase)->memorygfx)
	{
	    struct TagItem bm_create_tags[] =
	    {
		{ aHidd_BitMap_GfxHidd , (IPTR)CDD(GfxBase)->memorygfx },
		{ aHidd_PlanarBM_BitMap, 0			       },
		{ TAG_DONE	       , 0			       }
	    };

	    CDD(GfxBase)->planarbm_cache = create_object_cache(NULL, CLID_Hidd_PlanarBM, bm_create_tags, GfxBase);
	    DEBUG_INIT(bug("[driver_init] Planar bitmap cache 0x%p\n", CDD(GfxBase)->planarbm_cache));

	    if (CDD(GfxBase)->planarbm_cache)
	    {
		CDD(GfxBase)->gc_cache = create_object_cache(NULL, CLID_Hidd_GC, NULL, GfxBase);
		DEBUG_INIT(bug("[driver_init] GC cache 0x%p\n", CDD(GfxBase)->planarbm_cache));

		if (CDD(GfxBase)->gc_cache)
		    ReturnInt("driver_init", int, TRUE);

		delete_object_cache(CDD(GfxBase)->planarbm_cache, GfxBase);
	    }
	    OOP_DisposeObject(CDD(GfxBase)->memorygfx);
	}
    }

    ReturnInt("driver_init", int, FALSE);
}

/* Called after DOS is up & running */
static OOP_Object *create_framebuffer(struct monitor_driverdata *mdd, struct GfxBase *GfxBase)
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
    	D(bug("!!! create_framebuffer(): COULD NOT GET HIDD MODEID !!!\n"));
    } else {
    	/* Create the framebuffer object */
	fbtags[1].ti_Data = hiddmode;
	fb = HIDD_Gfx_NewBitMap(mdd->gfxhidd, fbtags);
    }

    return fb;
}

/*
 * Set up graphics display driver.
 * Creates necessary OS structures around it.
 *
 * gfxhidd - newly created driver object
 * result  - master driver structure
 */

struct monitor_driverdata *driver_Setup(OOP_Object *gfxhidd, struct GfxBase *GfxBase)
{
    ULONG cnt = 0;
    IPTR hwcursor = 0;
    IPTR fbtype = vHidd_FrameBuffer_Direct; /* We default to direct for historical reasons */
    UWORD compose = 0;
    BOOL can_compose = FALSE;
    BOOL ok = TRUE;
    ULONG i;
    HIDDT_ModeID *modes, *m;
    struct monitor_driverdata *mdd;
    struct HIDD_ModeProperties props = {0};

    D(bug("[driver_Setup] gfxhidd=0x%p (%s)\n", gfxhidd, OOP_OCLASS(gfxhidd)->ClassNode.ln_Name));

    modes = HIDD_Gfx_QueryModeIDs(gfxhidd, NULL);
    if (!modes)
        return NULL;

    /* Count number of display modes */
    for (m = modes; *m != vHidd_ModeID_Invalid; m ++)
	cnt++;

    mdd = AllocVec(sizeof(struct monitor_driverdata) + cnt * sizeof(struct DisplayInfoHandle), MEMF_PUBLIC|MEMF_CLEAR);
    D(bug("[driver_Setup] Allocated driverdata at 0x%p\n", mdd));
    if (!mdd)
    {
        HIDD_Gfx_ReleaseModeIDs(gfxhidd, modes);
        return NULL;
    }

    mdd->gfxhidd_orig = gfxhidd;

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
	mdd->modes[i].drv = mdd;

	/* Watch out! The last record in the array is terminator (vHidd_ModeID_Invalid) */
	if ((i < cnt) && (!compose))
	{
	    HIDD_Gfx_ModeProperties(gfxhidd, modes[i], &props, sizeof(props));
	    compose |= props.CompositionFlags;

	    if (!can_compose)
	    {
		OOP_Object *sync, *pf;
		IPTR colmod;

		HIDD_Gfx_GetMode(gfxhidd, modes[i], &sync, &pf);
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

    HIDD_Gfx_ReleaseModeIDs(gfxhidd, modes);

    /* Query properties of our driver */
#ifndef FORCE_SOFTWARE_SPRITE
    OOP_GetAttr(gfxhidd, aHidd_Gfx_HWSpriteTypes, &hwcursor);
#endif
    OOP_GetAttr(gfxhidd, aHidd_Gfx_FrameBufferType, &fbtype);

    if (hwcursor)
    {
        mdd->gfxhidd = gfxhidd;
    }
    else
    {
	D(bug("[driver_Setup] Hardware mouse cursor is not supported, using fakegfx.hidd\n"));

	mdd->gfxhidd = init_fakegfxhidd(gfxhidd, GfxBase);
	if (mdd->gfxhidd)
	    mdd->flags |= DF_UseFakeGfx;
	else
	    ok = FALSE;
    }

    if (ok)
    {
	D(bug("[driver_Setup] Ok, framebuffer type %ld\n", fbtype));

        /*
         * Instantiate framebuffer if needed.
         * Note that we perform this operation on fakegfx.hidd if it was plugged in.
         * This enables software mouse sprite on a framebuffer.
         */
        switch (fbtype)
        {
        case vHidd_FrameBuffer_Direct:
            mdd->flags |= DF_DirectFB;

        case vHidd_FrameBuffer_Mirrored:
	    mdd->framebuffer = create_framebuffer(mdd, GfxBase);
            break;
	}

	if ((fbtype == vHidd_FrameBuffer_None) || mdd->framebuffer)
	{
	    D(bug("[driver_Setup] FRAMEBUFFER OK: %p\n", mdd->framebuffer));

	    if ((!compose) && can_compose)
	    {
		D(bug("[driver_Setup] Software screen composition required\n"));

		mdd->flags |= DF_SoftCompose;
		compositor_Setup(mdd, GfxBase);
	    }

	    return mdd;
	}

	if (mdd->framebuffer)
	    OOP_DisposeObject(mdd->framebuffer);
    } /* if (fake gfx stuff ok) */

    if (mdd->flags & DF_UseFakeGfx)
	OOP_DisposeObject(mdd->gfxhidd);

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

void driver_Expunge(struct monitor_driverdata *mdd, struct GfxBase *GfxBase)
{
    /* Notify Intuition */
    if (CDD(GfxBase)->DriverNotify)
	CDD(GfxBase)->DriverNotify(mdd->userdata, FALSE, CDD(GfxBase)->notify_data);

    /* Dispose associated stuff */
    OOP_DisposeObject(mdd->compositor);
    OOP_DisposeObject(mdd->framebuffer);

    if (mdd->flags & DF_UseFakeGfx)
        OOP_DisposeObject(mdd->gfxhidd);

    /* Dispose driver object. This will take care about syncs etc */
    OOP_DisposeObject(mdd->gfxhidd_orig );

    FreeVec(mdd);
}

#if 0 /* Unused? */

static ULONG getbitmappixel(struct BitMap *bm
	, LONG x
	, LONG y
	, UBYTE depth
	, UBYTE plane_mask)
{
    UBYTE i;
    ULONG idx;

    ULONG mask;
    ULONG pen = 0;

    idx = COORD_TO_BYTEIDX(x, y, bm->BytesPerRow);
    mask = XCOORD_TO_MASK( x );
    
    for (i = depth - 1; depth; i -- , depth -- )
    {
        pen <<= 1; /* stegerg: moved to here, was inside if!? */

        if ((1L << i) & plane_mask)
	{
	    UBYTE *plane = bm->Planes[i];
	
	    if (plane == (PLANEPTR)-1)
	    {
	        pen |= 1;
	    }
	    else if (plane != NULL)
	    {
		if ((plane[idx] & mask) != 0)
		    pen |= 1;
	    }
	}

    }
    return pen;
}

#endif

/* This is called for every ViewPorts chain during MrgCop() */
ULONG driver_PrepareViewPorts(struct HIDD_ViewPortData *vpd, struct View *v, struct monitor_driverdata *mdd, struct GfxBase *GfxBase)
{
    /* Do not bother the driver if there's nothing to prepare. Can be changed if needed. */
    return vpd ? HIDD_Gfx_PrepareViewPorts(mdd->gfxhidd, vpd, v) : MCOP_OK;
}

/*
 * This is called for every ViewPorts chain during LoadView()
 * LoadView() actually has no rights for errors because error condition
 * in it will screw up the display, likely with no chance to recover.
 * Because of this we always return zero here.
 */
ULONG driver_LoadViewPorts(struct HIDD_ViewPortData *vpd, struct View *v, struct monitor_driverdata *mdd, struct GfxBase *GfxBase)
{
    struct BitMap *bitmap;
    OOP_Object *bm, *fb;
    BOOL compositing = FALSE;

    DEBUG_LOADVIEW(bug("[driver_LoadViewPorts] Showing ViewPortData 0x%p, BitMap object 0x%p\n", vpd, vpd ? vpd->Bitmap : NULL));
    mdd->display = vpd;

    /* First try the new method */
    if (HIDD_Gfx_ShowViewPorts(mdd->gfxhidd, vpd))
    {
        DEBUG_LOADVIEW(bug("[driver_LoadViewPorts] ShowViewPorts() worked\n"));
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
    DEBUG_LOADVIEW(bug("[driver_LoadViewPorts] Old bitmap 0x%p, New bitmap 0x%p, object 0x%p\n", mdd->frontbm, bitmap, bm));

    if (mdd->compositor)
    {
    	/*
    	 * Compositor present. Give ViewPorts chain to it.
    	 * For improved visual appearance it will call Show itself and return its result.
    	 * The compositor is expected to handle all possible failures internally. If some
    	 * internal error happens, it must fail back to single HIDD_Gfx_Show().
    	 */
    	fb = compositor_LoadViewPorts(mdd->compositor, vpd, &compositing, GfxBase);
    	DEBUG_LOADVIEW(bug("[driver_LoadViewPorts] Compositor returned 0x%p, active: %d\n", fb, compositing));
    }
    else
    {
    	/*
     	 * First check if the bitmap is already displayed. If so, do nothing (because
     	 * displaying the same bitmap twice may cause some problems)
     	 */
    	if (mdd->frontbm == bitmap)
            return 0;

    	fb = HIDD_Gfx_Show(mdd->gfxhidd, bm, fHidd_Gfx_Show_CopyBack);
    	DEBUG_LOADVIEW(bug("[driver_LoadViewPorts] Show() returned 0x%p\n", fb));
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
	UninstallFB(mdd, GfxBase);

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
	    InstallFB(mdd, GfxBase);

        /* Tell the driver to refresh the screen */
        OOP_GetAttr(fb, aHidd_BitMap_Width, &width);
        OOP_GetAttr(fb, aHidd_BitMap_Height, &height);
        DEBUG_LOADVIEW(bug("[driver_LoadViewPorts] Updating display, new size: %d x %d\n", width, height));

        HIDD_BM_UpdateRect(fb, 0, 0, width, height);
    }
    else
    {
    	/*
    	 * Screen is empty, simply reset the current bitmap.
    	 * Only framebuffer-less driver can return NULL. So we don't need
    	 * to call UninstallFB() here.
    	 */
        mdd->frontbm = NULL;
    }

    return 0;
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
void InstallFB(struct monitor_driverdata *mdd, struct GfxBase *GfxBase)
{
    if ((mdd->flags & DF_DirectFB) && mdd->frontbm)
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

    	DEBUG_LOADVIEW(bug("[driver_LoadViewPorts] Installed framebuffer: BitMap 0x%p, object 0x%p\n", mdd->frontbm, mdd->bm_bak));
    }
}

void UninstallFB(struct monitor_driverdata *mdd, struct GfxBase *GfxBase)
{
    if (mdd->bm_bak)
    {
        /* Put back the old values into the old bitmap */
        struct BitMap *oldbm = mdd->frontbm;

        DEBUG_LOADVIEW(bug("[driver_LoadViewPorts] Uninstalling framebuffer: BitMap 0x%p, object 0x%p\n", mdd->frontbm, mdd->bm_bak));

        HIDD_BM_OBJ(oldbm)    = mdd->bm_bak;
        HIDD_BM_COLMOD(oldbm) = mdd->colmod_bak;
        HIDD_BM_COLMAP(oldbm) = mdd->colmap_bak;

	/* No framebuffer installed */
        mdd->bm_bak = NULL;
    }
}

/* Find the first visible ViewPortData for the specified monitor in the View */
struct HIDD_ViewPortData *driver_FindViewPorts(struct View *view, struct monitor_driverdata *mdd, struct GfxBase *GfxBase)
{
    struct ViewPort *vp;

    for (vp = view->ViewPort; vp; vp = vp->Next)
    {
	if (!(vp->Modes & VP_HIDE))
	{
	    struct ViewPortExtra *vpe = (struct ViewPortExtra *)GfxLookUp(vp);

	    if (VPE_DRIVER(vpe) == mdd)
	    	return VPE_DATA(vpe);
	}
    }
    return NULL;
}

/*
 * Iterate through HIDD_ViewPortData chains in the view and call
 * the specified function for every chain and every driver.
 * The function will be called at least once for every driver.
 * If there is no ViewPorts for the driver, the function will be
 * called with ViewPortData = NULL.
 */
ULONG DoViewFunction(struct View *view, VIEW_FUNC fn, struct GfxBase *GfxBase)
{
    struct monitor_driverdata *mdd;
    ULONG rc = 0;

    ObtainSemaphoreShared(&CDD(GfxBase)->displaydb_sem);

    for (mdd = CDD(GfxBase)->monitors; mdd; mdd = mdd->next)
    {
        struct HIDD_ViewPortData *vpd = NULL;

        /*
         * Find the first visible ViewPort for this display. It
	 * will be a start of bitmaps chain to process.
	 */
	if (view)
	    vpd = driver_FindViewPorts(view, mdd, GfxBase);

	rc = fn(vpd, view, mdd, GfxBase);

	/* Interrupt immediately if the callback returned error */
	if (rc)
	    break;
    }

    ReleaseSemaphore(&CDD(GfxBase)->displaydb_sem);

    return rc;
}
