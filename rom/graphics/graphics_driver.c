/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Driver for using gfxhidd for gfx output
    Lang: english
*/

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/layers.h>
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

#include <hidd/graphics.h>

#include <cybergraphx/cybergraphics.h>

#include <stdio.h>
#include <string.h>

#include "graphics_intern.h"
#include "fakegfxhidd.h"
#include "intregions.h"
#include "dispinfo.h"
#include "gfxfuncsupport.h"
#include "fontsupport.h"

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

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


/* InitDriverData() just allocates memory for the struct. To use e.g.   */
/* AreaPtrns, UpdateAreaPtrn() has to allocate the memory for the       */
/* Pattern itself (and free previously used memory!)                    */

static inline void AddDriverDataToList(struct gfx_driverdata *dd, struct GfxBase * GfxBase)
{
    LONG hash;
    
    hash = CalcHashIndex((IPTR)dd, DRIVERDATALIST_HASHSIZE);
    
//  ObtainSemaphore(&PrivGBase(GfxBase)->driverdatasem);
    AddTail((struct List *)&PrivGBase(GfxBase)->driverdatalist[hash], (struct Node *)&dd->dd_Node);    
//  ReleaseSemaphore(&PrivGBase(GfxBase)->driverdatasem);
    
}

static inline void RemoveDriverDataFromList(struct gfx_driverdata *dd, struct GfxBase *GfxBase)
{
//  ObtainSemaphore(&PrivGBase(GfxBase)->driverdatasem);
    Remove((struct Node *)&dd->dd_Node);    
//  ReleaseSemaphore(&PrivGBase(GfxBase)->driverdatasem);
}

static inline BOOL FindDriverData(struct gfx_driverdata *dd, struct RastPort *rp, struct GfxBase *GfxBase)
{
    struct gfx_driverdata *hn = NULL;
    LONG    	     	  hash;
    BOOL    	    	  retval = FALSE;
    
    hash = CalcHashIndex((IPTR)dd, DRIVERDATALIST_HASHSIZE);

//  ObtainSemaphore(&PrivGBase(GfxBase)->driverdatasem);    
    ForeachNode((struct List *)&PrivGBase(GfxBase)->driverdatalist[hash], hn)
    {
    	if ((hn == dd) && (hn->dd_RastPort == rp))
	{
	    retval = TRUE;
	    break;
	}
    }
//  ReleaseSemaphore(&PrivGBase(GfxBase)->driverdatasem);
    
    return retval;
}

BOOL ObtainDriverData(struct RastPort *rp, struct GfxBase *GfxBase)
{
    struct gfx_driverdata *dd;
    
    if (RP_BACKPOINTER(rp) == rp)
    {
    	if (rp->Flags & RPF_SELF_CLEANUP)
	{
	    if (RP_DRIVERDATA(rp)) return TRUE;
	}
    }
    else
    {
    	/* We have detected a manually cloned rastport. Mark it as 'valid'
	   (backpointer to itself) but with NULL driverdata and non-self
	   cleanup */
    	RP_DRIVERDATA(rp) = NULL;
	rp->Flags &= ~RPF_SELF_CLEANUP;
	RP_BACKPOINTER(rp) = rp;
    }
    
    ObtainSemaphore(&PrivGBase(GfxBase)->driverdatasem);    
    dd = RP_DRIVERDATA(rp);
    if (dd)
    {
    	if (!(rp->Flags & RPF_SELF_CLEANUP) && FindDriverData(dd, rp, GfxBase))
	{
	    dd->dd_LockCount++;
	}
	else
	{
	    RP_DRIVERDATA(rp) = NULL;
	    dd = NULL;
	}
    }
    
    if (!dd)
    {
    	dd = AllocPooled(PrivGBase(GfxBase)->driverdatapool, sizeof(*dd));
	if (dd)
	{
	    struct monitor_driverdata *sdd;
	    struct TagItem gc_tags[] = {{ TAG_DONE}};

	    if (rp->BitMap)
	        sdd = GET_BM_DRIVERDATA(rp->BitMap);
	    else
	        sdd = (struct monitor_driverdata *)CDD(GfxBase);

	    dd->dd_GC = HIDD_Gfx_NewGC(sdd->gfxhidd, gc_tags);
	    if (dd->dd_GC)
	    {
   	    	dd->dd_RastPort = rp;
		dd->dd_LockCount = 1;
		
    	    	RP_DRIVERDATA(rp) = dd;
	    	rp->Flags |= RPF_DRIVER_INITED;

    	    	if (!(rp->Flags & RPF_SELF_CLEANUP)) AddDriverDataToList(dd, GfxBase);
		
		if (rp->BitMap) SetABPenDrMd(rp, (UBYTE)rp->FgPen, (UBYTE)rp->BgPen, rp->DrawMode);
	    }
    	    else
	    {
	    	FreePooled(PrivGBase(GfxBase)->driverdatapool, dd, sizeof(*dd));
		dd = NULL;
	    }
	    
	} /* if (dd) */
	
    } /* if (!dd) */
    
    ReleaseSemaphore(&PrivGBase(GfxBase)->driverdatasem);    
    
    return dd ? TRUE : FALSE;
}

void ReleaseDriverData(struct RastPort *rp, struct GfxBase *GfxBase)
{
    struct gfx_driverdata *dd = GetDriverData(rp);
    
    if (!(rp->Flags & RPF_SELF_CLEANUP))
    {
    /* FIXME: stegerg 23 jan 2004: needs semprotection, too! */
	/* CHECKME: stegerg 23 feb 2005: really?? */
	
    dd->dd_LockCount--;
    
    	// Don't do this:
	//
//    if (!dd->dd_LockCount) KillDriverData(rp, GfxBase);
	//
	// some garbage collection should later hunt for dd's with
	// 0 lockcount and possibly non-usage for a certain amount
	// of time and get rid of them.
    }
}

void KillDriverData(struct RastPort *rp, struct GfxBase *GfxBase)
{
    if (RP_BACKPOINTER(rp) == rp)
    {
        struct gfx_driverdata *dd = NULL;
    
    	if (rp->Flags & RPF_SELF_CLEANUP)
	{
	    dd = RP_DRIVERDATA(rp);
	}
    	else
    	{
    ObtainSemaphore(&PrivGBase(GfxBase)->driverdatasem);    
    	    if (FindDriverData(RP_DRIVERDATA(rp), rp, GfxBase))
	    {
	    	dd = RP_DRIVERDATA(rp);
	    	RemoveDriverDataFromList(dd, GfxBase);
	    }
    	    ReleaseSemaphore(&PrivGBase(GfxBase)->driverdatasem);    
    	}
    
    if (dd)
    {
    	struct monitor_driverdata *sdd;

    	/* rp->BitMap may not be valid anymore! */
	if (0) // (rp->BitMap)
	    sdd = GET_BM_DRIVERDATA(rp->BitMap);
	else
	    sdd = (struct monitor_driverdata *)CDD(GfxBase);

    	HIDD_Gfx_DisposeGC(sdd->gfxhidd, dd->dd_GC);
	FreePooled(PrivGBase(GfxBase)->driverdatapool, dd, sizeof(*dd)); 
	    RP_DRIVERDATA(rp) = NULL;
    }
    
    }
        
}

int driver_init(struct GfxBase * GfxBase)
{

    EnterFunc(bug("driver_init()\n"));

    /* Our underlying RTG subsystem core must be already up and running */
    if (!OpenLibrary("graphics.hidd", 0))
        return FALSE;

    /* Initialize the semaphore used for the chunky buffer */
    InitSemaphore(&(PrivGBase(GfxBase)->pixbuf_sema));
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
	PrivGBase(GfxBase)->pixel_buf=AllocMem(PIXELBUF_SIZE,MEMF_ANY);
	if (PrivGBase(GfxBase)->pixel_buf) {

	    /* Init display mode database */
	    InitSemaphore(&CDD(GfxBase)->displaydb_sem);
	    CDD(GfxBase)->invalid_id = INVALID_ID;
	    CDD(GfxBase)->last_id = AROS_RTG_MONITOR_ID;

	    /* Init memory driver */
	    CDD(GfxBase)->memorygfx = OOP_NewObject(NULL, CLID_Hidd_Gfx, NULL);
	    DEBUG_INIT(bug("[driver_init] Memory driver object 0x%p\n", CDD(GfxBase)->memorygfx));
	    if (CDD(GfxBase)->memorygfx) {
		struct TagItem bm_create_tags[] = {
		    { aHidd_BitMap_GfxHidd	, (IPTR)CDD(GfxBase)->memorygfx },
		    { aHidd_PlanarBM_AllocPlanes, FALSE				 },
		    { TAG_DONE			, 0UL				 }
		};

	    	CDD(GfxBase)->planarbm_cache = create_object_cache(NULL, CLID_Hidd_PlanarBM, bm_create_tags, GfxBase);
		DEBUG_INIT(bug("[driver_init] Planar bitmap cache 0x%p\n", CDD(GfxBase)->planarbm_cache));
		if (CDD(GfxBase)->planarbm_cache) {
		    struct TagItem gc_create_tags[] = { { TAG_DONE, 0UL } };

		    CDD(GfxBase)->gc_cache = create_object_cache(NULL, CLID_Hidd_GC, gc_create_tags, GfxBase);
		    DEBUG_INIT(bug("[driver_init] GC cache 0x%p\n", CDD(GfxBase)->planarbm_cache));
		    if (CDD(GfxBase)->gc_cache)
		        ReturnInt("driver_init", int, TRUE);
		    delete_object_cache(CDD(GfxBase)->planarbm_cache, GfxBase);
		    
		}
		OOP_DisposeObject(CDD(GfxBase)->memorygfx);
	    }
	    FreeMem(PrivGBase(GfxBase)->pixel_buf, PIXELBUF_SIZE);
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
    IPTR noframebuffer = 0;
    BOOL ok = TRUE;
    ULONG i;
    HIDDT_ModeID *modes, *m;
    struct monitor_driverdata *mdd;

    D(bug("[driver_Setup] gfxhidd=0x%p\n", gfxhidd));

    modes = HIDD_Gfx_QueryModeIDs(gfxhidd, NULL);
    if (!modes)
        return NULL;

    /* Count number of display modes */
    for (m = modes; *m != vHidd_ModeID_Invalid; m ++)
	cnt++;

    mdd = AllocVec(sizeof(struct monitor_driverdata) + cnt * sizeof(struct DisplayInfoHandle), MEMF_PUBLIC|MEMF_CLEAR);
    D(bug("[driver_Setup] Allocated driverdata at 0x%p\n", mdd));
    if (!mdd) {
        HIDD_Gfx_ReleaseModeIDs(gfxhidd, modes);
        return NULL;
    }

    mdd->gfxhidd_orig = gfxhidd;

    /* Fill in ModeID database in the driverdata */
    for (i = 0; i <= cnt; i++) {
        mdd->modes[i].id = modes[i];
	mdd->modes[i].drv = mdd;
    }

    HIDD_Gfx_ReleaseModeIDs(gfxhidd, modes);

#ifndef FORCE_SOFTWARE_SPRITE
    OOP_GetAttr(gfxhidd, aHidd_Gfx_HWSpriteTypes, &hwcursor);
#endif
    OOP_GetAttr(gfxhidd, aHidd_Gfx_NoFrameBuffer, &noframebuffer);

    if (hwcursor) {
        mdd->gfxhidd = gfxhidd;
    } else {
	D(bug("[driver_Setup] Hardware mouse cursor is not supported, using fakegfx.hidd\n"));

	mdd->gfxhidd = init_fakegfxhidd(gfxhidd, GfxBase);
	if (mdd->gfxhidd)
	    mdd->flags |= DF_UseFakeGfx;
	else
	    ok = FALSE;
    }

    if (ok) {
	struct TagItem gc_create_tags[] = { { TAG_DONE, 0UL } };

	D(bug("[driver_Setup] Ok\n"));

        /* FIXME: perhaps driver should be able to supply own GC class? */
	mdd->gc_cache = create_object_cache(NULL, CLID_Hidd_GC, gc_create_tags, GfxBase);
	if (mdd->gc_cache) {
	    D(bug("[driver_Setup] GC Cache created\n"));

	    if (!noframebuffer)
	        /* Note that we perform this operation on fakegfx.hidd if it was
		   plugged in */
		mdd->framebuffer = create_framebuffer(mdd, GfxBase);

	    if (noframebuffer || mdd->framebuffer) {
		D(bug("[driver_Setup] FRAMEBUFFER OK: %p\n", mdd->framebuffer));
		return mdd;
	    }

	    if (mdd->framebuffer)
		OOP_DisposeObject(mdd->framebuffer);

	    delete_object_cache(mdd->gc_cache, GfxBase);
	} /* if (gc object cache ok) */
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
    if (CDD(GfxBase)->DriverNotify)
	CDD(GfxBase)->DriverNotify(mdd->userdata, FALSE, CDD(GfxBase)->notify_data);

    if (mdd->framebuffer)
	OOP_DisposeObject(mdd->framebuffer);

    if (mdd->gc_cache)
	delete_object_cache(mdd->gc_cache, GfxBase );

    if (mdd->flags & DF_UseFakeGfx)
        OOP_DisposeObject(mdd->gfxhidd);

    /* Dispose driver object. This will take care about syncs etc */
    if (mdd->gfxhidd_orig)
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

static void driver_LoadViewPorts(struct HIDD_ViewPortData *vpd, struct View *v, struct monitor_driverdata *mdd, struct GfxBase *GfxBase)
{
    struct BitMap *bitmap;
    OOP_Object *bm, *fb;
    OOP_Object *cmap, *pf;
    HIDDT_ColorModel colmod;

    DEBUG_LOADVIEW(bug("[driver_LoadViewPorts] Showing ViewPortData 0x%p, BitMap object 0x%p\n", vpd, vpd ? vpd->Bitmap : NULL));
    mdd->display = vpd;

    /* First try the new method */
    if (HIDD_Gfx_ShowViewPorts(mdd->gfxhidd, vpd, v))
    {
        DEBUG_LOADVIEW(bug("[driver_LoadViewPorts] ShowViewPorts() worked\n"));
        return;
    }

    /* If it failed, we may be working with a framebuffer. First check if the bitmap
    is already displayed. If so, do nothing (because displaying the same bitmap twice may
    cause some problems */
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

    if (mdd->frontbm == bitmap)
        return;

    fb = HIDD_Gfx_Show(mdd->gfxhidd, bm, fHidd_Gfx_Show_CopyBack);
    DEBUG_LOADVIEW(bug("[driver_LoadViewPorts] Show() returned 0x%p\n", fb));

    /* Summary of possible responses (when no error happened):
       NoFrameBuffer = FALSE: bm = NULL  -> fb != NULL and bm != fb
       NoFrameBuffer = FALSE: bm != NULL -> fb != NULL and bm != fb
       NoFrameBuffer = TRUE : bm = NULL  -> fb == NULL and bm == fb
       NoFrameBuffer = TRUE : bm != NULL -> fb != NULL and bm == fb
    */
    if (fb)
    {
        IPTR width, height;

        /* Do not swap bitmaps for NoFrameBuffer drivers */
        /* Detection: these kind of drivers must return the same bitmap they received */
        if (fb != bm)
        {
            /*
             * FIXME: THIS IS NOT THREADSAFE
             * To make this threadsafe we have to lock
             * all gfx access in all the rendering calls
             */

            DEBUG_LOADVIEW(bug("[driver_LoadViewPorts] Replacing framebuffer\n"));

            /* Set this as the active screen */
            if (NULL != mdd->frontbm)
            {
                struct BitMap *oldbm;

                /* Put back the old values into the old bitmap */
                oldbm = mdd->frontbm;
                HIDD_BM_OBJ(oldbm)      = mdd->bm_bak;
                HIDD_BM_COLMOD(oldbm)   = mdd->colmod_bak;
                HIDD_BM_COLMAP(oldbm)   = mdd->colmap_bak;
            }

            mdd->bm_bak     = bm;
            mdd->colmod_bak = bitmap ? HIDD_BM_COLMOD(bitmap) : 0;
            mdd->colmap_bak = bitmap ? HIDD_BM_COLMAP(bitmap) : NULL;

            if (bitmap)
            {
                /* Insert the framebuffer in its place */
                OOP_GetAttr(fb, aHidd_BitMap_ColorMap, (IPTR *)&cmap);
                OOP_GetAttr(fb, aHidd_BitMap_PixFmt, (IPTR *)&pf);
                OOP_GetAttr(pf, aHidd_PixFmt_ColorModel, &colmod);

                HIDD_BM_OBJ(bitmap)     = fb;
                HIDD_BM_COLMOD(bitmap)  = colmod;
                HIDD_BM_COLMAP(bitmap)  = cmap;
            }
        }

        /* We need to always remember our new frontmost bitmap, even if we do not work
           with a framebuffer */
        mdd->frontbm = bitmap;

        /* Tell the driver to refresh the screen */
        OOP_GetAttr(fb, aHidd_BitMap_Width, &width);
        OOP_GetAttr(fb, aHidd_BitMap_Height, &height);
        DEBUG_LOADVIEW(bug("[driver_LoadViewPorts] Updating framebuffer, new size: %d x %d\n", width, height));

        HIDD_BM_UpdateRect(fb, 0, 0, width, height);
    }
    else
        mdd->frontbm = NULL;
}

void driver_LoadView(struct View *view, struct GfxBase *GfxBase)
{
    struct monitor_driverdata *mdd;

    ObtainSemaphoreShared(&CDD(GfxBase)->displaydb_sem);

    for (mdd = CDD(GfxBase)->monitors; mdd; mdd = mdd->next)
    {
        struct HIDD_ViewPortData *vpd = NULL;

        /* Find the first visible ViewPort for this display. It
	   will be a start of bitmaps chain to show. */
	if (view)
	{
	    struct ViewPort *vp;

            for (vp = view->ViewPort; vp; vp = vp->Next)
            {
		if (!(vp->Modes & VP_HIDE))
		{
		    struct ViewPortExtra *vpe = (struct ViewPortExtra *)GfxLookUp(vp);

		    if (VPE_DRIVER(vpe) == mdd)
		    {
		    	vpd = VPE_DATA(vpe);
			break;
		    }
		}
	    }
	}

	driver_LoadViewPorts(vpd, view, mdd, GfxBase);
    }
    
    ReleaseSemaphore(&CDD(GfxBase)->displaydb_sem);
}
