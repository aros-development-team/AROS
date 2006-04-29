/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
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
#include "graphics_internal.h"
#include "intregions.h"
#include "dispinfo.h"
#include "gfxfuncsupport.h"
#include "fontsupport.h"

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>


static BOOL init_cursor(struct GfxBase *GfxBase);
static VOID cleanup_cursor(struct GfxBase *GfxBase);

struct rgbpix_render_data
{
    HIDDT_Pixel pixel;
};


static LONG rgbpix_write(APTR pr_data, OOP_Object *bm, OOP_Object *gc,
    	            	 LONG x, LONG y, struct GfxBase *GfxBase)
{
    struct rgbpix_render_data *prd;
    
    prd = (struct rgbpix_render_data *)pr_data;
    
    HIDD_BM_PutPixel(bm, x, y, prd->pixel);
    
    return 0;
}
#if 0
OOP_AttrBase HiddBitMapAttrBase	= 0;
OOP_AttrBase HiddGCAttrBase		= 0;
OOP_AttrBase HiddSyncAttrBase	= 0;
OOP_AttrBase HiddPixFmtAttrBase	= 0;
OOP_AttrBase HiddPlanarBMAttrBase	= 0; 
OOP_AttrBase HiddGfxAttrBase	= 0; 
OOP_AttrBase HiddFakeGfxHiddAttrBase	= 0;

static struct OOP_ABDescr attrbases[] = {
    { IID_Hidd_BitMap,		&HiddBitMapAttrBase	},
    { IID_Hidd_GC,		&HiddGCAttrBase		},
    { IID_Hidd_Sync,		&HiddSyncAttrBase	},
    { IID_Hidd_PixFmt,		&HiddPixFmtAttrBase	},
    { IID_Hidd_PlanarBM,	&HiddPlanarBMAttrBase	},
    { IID_Hidd_Gfx,		&HiddGfxAttrBase	},
    { IID_Hidd_FakeGfxHidd,	&HiddFakeGfxHiddAttrBase	},
    { NULL, 0UL }
};

#endif


struct ETextFont
{
    struct TextFont	etf_Font;
};


/* InitDriverData() just allocates memory for the struct. To use e.g.   */
/* AreaPtrns, UpdateAreaPtrn() has to allocate the memory for the       */
/* Pattern itself (and free previously used memory!)                    */

#if NEW_DRIVERDATA_CODE

static inline LONG CalcDriverDataHash(APTR resource)
{
    LONG l1, l2, l3, l4, hash;
    
    /* FIXME: Probably sucks. I have no clue about this hash stuff */
    
    l1 = ((LONG)resource) & 0xFF;
    l2 = (((LONG)resource) >> 8) & 0xFF;
    l3 = (((LONG)resource) >> 16) & 0xFF;
    l4 = (((LONG)resource) >> 24) & 0xFF;
   
    hash = (l1 + l2 + l3 + l4) % DRIVERDATALIST_HASHSIZE;

    return hash;
}

static inline void AddDriverDataToList(struct gfx_driverdata *dd, struct GfxBase * GfxBase)
{
    LONG hash;
    
    hash = CalcDriverDataHash(dd);
    
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
    
    hash = CalcDriverDataHash(dd);

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
	    struct shared_driverdata *sdd;
	    struct TagItem gc_tags[] = {{ TAG_DONE}};

	    sdd = SDD(GfxBase);
		
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
    	struct shared_driverdata *sdd;
    	
	sdd = SDD(GfxBase);

    	HIDD_Gfx_DisposeGC(sdd->gfxhidd, dd->dd_GC);
	FreePooled(PrivGBase(GfxBase)->driverdatapool, dd, sizeof(*dd)); 
	    RP_DRIVERDATA(rp) = NULL;
    }
    
    }
        
}

#else

struct gfx_driverdata * obsolete_InitDriverData (struct RastPort * rp, struct GfxBase * GfxBase)
{
    struct gfx_driverdata * dd;
    EnterFunc(bug("InitDriverData(rp=%p)\n", rp));

    /* Does this rastport have a bitmap ? */
    //if (rp->BitMap)
    {
        D(bug("Got bitmap\n"));
        /* Displayable ? (== rastport of a screen) */
	//if (IS_HIDD_BM(rp->BitMap))
	{
            D(bug("Has HIDD bitmap (displayable)\n"));

	    /* We can use this rastport. Allocate driverdata */
    	    dd = AllocMem (sizeof(struct gfx_driverdata), MEMF_CLEAR);
    	    if (dd)
    	    {
	        struct shared_driverdata *sdd;
		struct TagItem gc_tags[] = {
		    { TAG_DONE, 	0UL}
		};
		
		
		D(bug("Got driverdata\n"));
		sdd = SDD(GfxBase);
		
		dd->dd_GC = HIDD_Gfx_NewGC(sdd->gfxhidd, gc_tags);
		if (dd->dd_GC)
		{

		    D(bug("Got GC HIDD object\n"));
    		    dd->dd_RastPort = rp;
    		    SetDriverData(rp, dd);
    		    rp->Flags |= RPF_DRIVER_INITED;

		    ReturnPtr("InitDriverData", struct gfx_driverdata *, dd);
	        }

		FreeMem(dd, sizeof (struct gfx_driverdata));
	
    	    }
	}
    }

    ReturnPtr("InitDriverData", struct gfx_driverdata *, NULL);
}

void obsolete_DeinitDriverData (struct RastPort * rp, struct GfxBase * GfxBase)
{
    struct gfx_driverdata * dd;
    struct shared_driverdata *sdd;
    
    EnterFunc(bug("DeInitDriverData(rp=%p)\n", rp));
		
    sdd = SDD(GfxBase);

    dd = RP_DRIVERDATA(rp);;
    RP_DRIVERDATA(rp) = 0;

    HIDD_Gfx_DisposeGC(sdd->gfxhidd, dd->dd_GC);

    FreeMem (dd, sizeof(struct gfx_driverdata));
    rp->Flags &= ~RPF_DRIVER_INITED;

    ReturnVoid("DeinitDriverData");
}

BOOL obsolete_CorrectDriverData (struct RastPort * rp, struct GfxBase * GfxBase)
{
    BOOL retval = TRUE;
    struct gfx_driverdata * dd, * old;

    if (!rp)
    {
	retval = FALSE;
    }
    else
    {
	old = GetDriverData(rp);
	if (!old)
	{
	    old = obsolete_InitDriverData(rp, GfxBase);

/* stegerg: ???? would have returned TRUE even if old == NULL
	    if (old)
	    	retval = TRUE;
*/
	    if (!old) retval = FALSE;
	}
	else if (rp != old->dd_RastPort)
	{
	    /* stegerg: cloned rastport?	    
	    ** Make sure to clear driverdata pointer and flag
	    ** in case InitDriverData fail
	    */
	    RP_DRIVERDATA(rp) = 0;
	    rp->Flags &= ~RPF_DRIVER_INITED;

	    dd = obsolete_InitDriverData(rp, GfxBase);

/* stegerg: ???? would have returned TRUE even if dd = NULL
	    if (dd)
	   	 retval = TRUE;
*/

	    if (!dd) retval = FALSE;

	}
    }
    
    return retval;
}

#endif

int driver_init(struct GfxBase * GfxBase)
{

    EnterFunc(bug("driver_init()\n"));
    
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

	    ReturnInt("driver_init", int, TRUE);

	    FreeMem(PrivGBase(GfxBase)->pixel_buf, PIXELBUF_SIZE);
	}
	
    }
	
    ReturnInt("driver_init", int, FALSE);
}

int driver_open (struct GfxBase * GfxBase)
{
    return TRUE;
}

void driver_close (struct GfxBase * GfxBase)
{
    return;
}

void driver_expunge (struct GfxBase * GfxBase)
{
    
    /* Try to free some other stuff */
    cleanup_cursor(GfxBase);

    if (SDD(GfxBase)->framebuffer)
	OOP_DisposeObject(SDD(GfxBase)->framebuffer);

#if 0
    if (SDD(GfxBase)->activescreen_inited)
	cleanup_activescreen_stuff(GfxBase);
#endif
    if (SDD(GfxBase)->dispinfo_db)
	destroy_dispinfo_db(SDD(GfxBase)->dispinfo_db, GfxBase);

    if ( SDD(GfxBase)->planarbm_cache )
	delete_object_cache( SDD(GfxBase)->planarbm_cache, GfxBase );

    if ( SDD(GfxBase)->gc_cache )
	delete_object_cache( SDD(GfxBase)->gc_cache, GfxBase );

    if ( SDD(GfxBase)->fakegfx_inited )
	cleanup_fakegfxhidd( &SDD(GfxBase)->fakegfx_staticdata, GfxBase);

    if ( SDD(GfxBase)->gfxhidd_orig )
	OOP_DisposeObject( SDD(GfxBase)->gfxhidd_orig );
	     
    return;
}

/* Called after DOS is up & running */
static OOP_Object *create_framebuffer(struct GfxBase *GfxBase)
{
    struct TagItem fbtags[] = {
    	{ aHidd_BitMap_FrameBuffer,	TRUE	},
	{ aHidd_BitMap_ModeID,		0	},
	{ TAG_DONE, 0 }
    };

    HIDDT_ModeID hiddmode;
    OOP_Object *fb = NULL;

    /* Get the highest available resolution at the best possible depth */
    hiddmode = get_best_resolution_and_depth(GfxBase);
    if (vHidd_ModeID_Invalid == hiddmode) {
    	D(bug("!!! create_framebuffer(): COULD NOT GET HIDD MODEID !!!\n"));
    } else {
    	/* Create the framebuffer object */
	fbtags[1].ti_Data = hiddmode;
	fb = HIDD_Gfx_NewBitMap(SDD(GfxBase)->gfxhidd, fbtags);
    }

    return fb;
}
const UBYTE def_pointer_shape[] =
{
    06,02,00,00,00,00,00,00,00,00,00,
    01,06,02,02,00,00,00,00,00,00,00,
    00,01,06,06,02,02,00,00,00,00,00,
    00,01,06,06,06,06,02,02,00,00,00,
    00,00,01,06,06,06,06,06,02,02,00,
    00,00,01,06,06,06,06,06,06,06,00,
    00,00,00,01,06,06,06,02,00,00,00,
    00,00,00,01,06,06,01,06,02,00,00,
    00,00,00,00,01,06,00,01,06,02,00,
    00,00,00,00,01,06,00,00,01,06,02,
    00,00,00,00,00,00,00,00,00,01,06
};

#define DEF_POINTER_WIDTH	11
#define DEF_POINTER_HEIGHT	11
#define DEF_POINTER_DEPTH	7


static BOOL init_cursor(struct GfxBase *GfxBase)
{
    /* Create the pointer bitmap */
    struct TagItem pbmtags[] = {
    	{ aHidd_BitMap_Width,		DEF_POINTER_WIDTH		},
	{ aHidd_BitMap_Height,		DEF_POINTER_HEIGHT		},
	{ aHidd_BitMap_StdPixFmt,	vHidd_StdPixFmt_LUT8		},
	{ TAG_DONE, 0UL }
    };
    SDD(GfxBase)->pointerbm = HIDD_Gfx_NewBitMap(SDD(GfxBase)->gfxhidd, pbmtags);
    if (NULL != SDD(GfxBase)->pointerbm) {
	OOP_Object *gc;
	    
	gc = obtain_cache_object(SDD(GfxBase)->gc_cache, GfxBase);
	if (NULL != gc) {
	    /* Copy the default pointer image into the created pointer bitmap */
	    ULONG i;
	    struct TagItem gc_tags[] = {
		{ aHidd_GC_DrawMode,	vHidd_GC_DrawMode_Copy	},
		{ TAG_DONE, 0UL }
	    };
	    
	    HIDDT_Color col[DEF_POINTER_DEPTH];
	    
	    col[0].red		= 0x0000;
	    col[0].green	= 0x0000;
	    col[0].blue		= 0x0000;
	    col[0].alpha	= 0x0000;
	    col[1].red		= 0x0000;
	    col[1].green	= 0x0000;
	    col[1].blue		= 0x1200;
	    col[1].alpha	= 0x0000;
	    col[2].red		= 0xE0E0;
	    col[2].green	= 0xE0E0;
	    col[2].blue		= 0xC0C0;
	    col[2].alpha	= 0x0000;
	    col[6].red		= 0xE0E0;
	    col[6].green	= 0x4040;
	    col[6].blue		= 0x4040;
	    col[6].alpha	= 0x0000;

	    for (i = 3; i < 6; i ++) {
	    	col[i].red	= 0x0000;
		col[i].green	= 0x0000;
	    	col[i].blue	= 0x0000;
	    	col[i].alpha	= 0x0000;
	    }
	    for (i = 7; i < DEF_POINTER_DEPTH; i ++) {
	    	col[i].red	= 0x0000;
		col[i].green	= 0x0000;
	    	col[i].blue	= 0x0000;
	    	col[i].alpha	= 0x0000;
	    }
	    
	    HIDD_BM_SetColors(SDD(GfxBase)->pointerbm, col, 0, DEF_POINTER_DEPTH);

	    OOP_SetAttrs(gc, gc_tags);
#if 0
	    /* PutImageLUT not yet implemented in gfx baseclass */	    
	    HIDD_BM_PutImageLUT(SDD(GfxBase)->pointerbm, gc
			, (UBYTE *)def_pointer_shape
			, DEF_POINTER_WIDTH
			, 0, 0
			, DEF_POINTER_WIDTH, DEF_POINTER_HEIGHT
			, &plut
	    );
#else
	    HIDD_BM_PutImage(SDD(GfxBase)->pointerbm, gc
			, (UBYTE *)def_pointer_shape
			, DEF_POINTER_WIDTH
			, 0, 0
			, DEF_POINTER_WIDTH, DEF_POINTER_HEIGHT
			, vHidd_StdPixFmt_LUT8
	    );

#endif		
	    release_cache_object(SDD(GfxBase)->gc_cache, gc, GfxBase);
	    
	    if (HIDD_Gfx_SetCursorShape(SDD(GfxBase)->gfxhidd, SDD(GfxBase)->pointerbm)) {
D(bug("CURSOR SHAPE SET\n"));
		/* Make it visible */
		HIDD_Gfx_SetCursorVisible(SDD(GfxBase)->gfxhidd, TRUE);
		
	    	return TRUE;
	    }
	}
    }
    
    cleanup_cursor(GfxBase);

    return FALSE;
}

static VOID cleanup_cursor(struct GfxBase *GfxBase)
{
    if (NULL != SDD(GfxBase)->pointerbm) {
   	OOP_DisposeObject(SDD(GfxBase)->pointerbm);
	SDD(GfxBase)->pointerbm = NULL;
    }
}

BOOL driver_LateGfxInit (APTR data, struct GfxBase *GfxBase)
{

    /* Supplied data is really the librarybase of a HIDD */
    STRPTR gfxhiddname = (STRPTR)data;
    struct TagItem tags[] = {
    	{ TAG_DONE, 0UL },
    };
    EnterFunc(bug("driver_LateGfxInit(gfxhiddname=%s)\n", gfxhiddname));

    /* Create a new GfxHidd object */

    SDD(GfxBase)->gfxhidd = SDD(GfxBase)->gfxhidd_orig = OOP_NewObject(NULL, gfxhiddname, tags);
    D(bug("driver_LateGfxInit: gfxhidd=%p\n", SDD(GfxBase)->gfxhidd));

    if (NULL != SDD(GfxBase)->gfxhidd) {
	IPTR hwcursor;
	BOOL ok = TRUE;

    	/* Get method id here, to avoid any possible semaphore involment when calling the method */ 
    	SDD(GfxBase)->hiddGfxShowImminentReset_MethodID = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_ShowImminentReset);

	OOP_GetAttr(SDD(GfxBase)->gfxhidd, aHidd_Gfx_SupportsHWCursor, &hwcursor);
	SDD(GfxBase)->has_hw_cursor = (BOOL)hwcursor;
	if (!hwcursor) {
	    OOP_Object *fgh;
	    D(bug("There's no hardware cursor\n"));

	    fgh = init_fakegfxhidd(SDD(GfxBase)->gfxhidd
	    	, &SDD(GfxBase)->fakegfx_staticdata
		, GfxBase);

	    if (NULL != fgh) {
	    	SDD(GfxBase)->gfxhidd = fgh;
		SDD(GfxBase)->fakegfx_inited = TRUE;
	    } else {
	    	ok = FALSE;
	    }
	}

	if (ok) {
	    struct TagItem gc_create_tags[] = { { TAG_DONE, 0UL } };
	    D(bug("Ok\n"));
	    SDD(GfxBase)->gc_cache = create_object_cache(NULL, IID_Hidd_GC, gc_create_tags, GfxBase);
	    if (NULL != SDD(GfxBase)->gc_cache) {

		struct TagItem bm_create_tags[] = {
#warning Maybe make this class private and create the object through the graphicshidd
			{ aHidd_BitMap_GfxHidd,		(IPTR)SDD(GfxBase)->gfxhidd_orig },
			{ aHidd_BitMap_Displayable,	FALSE	},
			{ aHidd_PlanarBM_AllocPlanes,	FALSE },
			{ TAG_DONE, 0UL }
		};

		D(bug("Cache created\n"));
		SDD(GfxBase)->planarbm_cache = create_object_cache(NULL, CLID_Hidd_PlanarBM, bm_create_tags, GfxBase);

		if (NULL != SDD(GfxBase)->planarbm_cache) {

		    /* Move the modes into the displayinfo DB */
		    SDD(GfxBase)->dispinfo_db = build_dispinfo_db(GfxBase);
		    if (NULL != SDD(GfxBase)->dispinfo_db) {

			SDD(GfxBase)->framebuffer = create_framebuffer(GfxBase);
			if (NULL != SDD(GfxBase)->framebuffer) {
D(bug("FRAMEBUFFER OK: %p\n", SDD(GfxBase)->framebuffer));
			    if (init_cursor(GfxBase)) {

D(bug("MOUSE INITED\n"));
		            	ReturnBool("driver_LateGfxInit", TRUE);
			    }
			    OOP_DisposeObject(SDD(GfxBase)->framebuffer);
		    	} /* if (framebuffer inited) */
		    	destroy_dispinfo_db(SDD(GfxBase)->dispinfo_db, GfxBase);
		    	SDD(GfxBase)->dispinfo_db = NULL;
		    } /* if (displayinfo db inited) */
		    delete_object_cache(SDD(GfxBase)->planarbm_cache, GfxBase);
	            SDD(GfxBase)->planarbm_cache = NULL;
	    	} /* if (planarbm cache created) */
	    	delete_object_cache(SDD(GfxBase)->gc_cache, GfxBase);
		SDD(GfxBase)->gc_cache = NULL;
	    } /* if (gc object cache ok) */
	    
	} /* if (fake gfx stuff ok) */

	if (SDD(GfxBase)->fakegfx_inited) {
	    cleanup_fakegfxhidd(&SDD(GfxBase)->fakegfx_staticdata, GfxBase);
	    SDD(GfxBase)->fakegfx_inited = FALSE;
	}
	OOP_DisposeObject(SDD(GfxBase)->gfxhidd_orig);
	SDD(GfxBase)->gfxhidd_orig = NULL;
	    
    }
    
    ReturnBool("driver_LateGfxInit", FALSE);

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
    ULONG pen = 0L;

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


#define dumprect(rect) \
        kprintf(#rect " : (%d,%d) - (%d,%d)\n",(rect)->MinX,(rect)->MinY,(rect)->MaxX,(rect)->MaxY)


struct bgf_render_data {
    WORD fbm_xsrc;
    OOP_Object *fbm;
};

static ULONG bgf_render(APTR bgfr_data
	, LONG srcx, LONG srcy
	, OOP_Object *dstbm_obj, OOP_Object *dst_gc
	, LONG x1, LONG y1, LONG x2, LONG y2
	, struct GfxBase *GfxBase)
{
    struct bgf_render_data *bgfrd;
    ULONG width, height;
    
    width  = x2 - x1 + 1;
    height = y2 - y1 + 1;
    
    ASSERT(width > 0 && height > 0);

    bgfrd = (struct bgf_render_data *)bgfr_data;
    
    HIDD_BM_BlitColorExpansion( dstbm_obj
    	, dst_gc
	, bgfrd->fbm
	, srcx + bgfrd->fbm_xsrc, srcy /* stegerg: instead of srcy there was a 0 */
	, x1, y1
	, width, height
     );
     
     return width * height;
    
}
    
void blit_glyph_fast(struct RastPort *rp, OOP_Object *fontbm, WORD xsrc
	, WORD destx, WORD desty, UWORD width, UWORD height
	, struct GfxBase * GfxBase)
{
    
    struct Rectangle rr;
    struct bgf_render_data bgfrd;

    EnterFunc(bug("blit_glyph_fast(%d, %d, %d, %d, %d)\n"
    	, xsrc, destx, desty, width, height));
	
    
    ASSERT(width > 0 && height > 0);
    
    bgfrd.fbm_xsrc = xsrc;
    bgfrd.fbm	   = fontbm;
    
    rr.MinX = destx;
    rr.MinY = desty;
    rr.MaxX = destx + width  - 1;
    rr.MaxY = desty + height - 1;
	
    if (!OBTAIN_DRIVERDATA(rp, GfxBase))
    	ReturnVoid("blit_glyph_fast");
	
    do_render_func(rp, NULL, &rr, bgf_render, &bgfrd, FALSE, GfxBase);
	
    RELEASE_DRIVERDATA(rp, GfxBase);
    
    ReturnVoid("blit_glyph_fast");
}

    

#define NUMCHARS(tf) ((tf->tf_HiChar - tf->tf_LoChar) + 2)
#define CTF(x) ((struct ColorTextFont *)x)

void driver_Text (struct RastPort * rp, CONST_STRPTR string, LONG len,
		  struct GfxBase * GfxBase)
{

#warning Does not handle color textfonts
    WORD  render_y;
    struct TextFont *tf;
    WORD current_x;
    OOP_Object *fontbm = NULL;
    
    if (!OBTAIN_DRIVERDATA(rp, GfxBase))
    	return;

    if ((rp->DrawMode & ~INVERSVID) == JAM2)
    {
    	struct TextExtent te;
    	ULONG 	    	  old_drmd = GetDrMd(rp);

    	/* This is actually needed, because below only the
	   part of the glyph which contains data is rendered:
	   
	   ...1100...
	   ...1100...
	   ...1100...
	   ...1111...
	   
	   '.' at left side can be there because of kerning.
	   '.' at the right side can be there because of
	   CharSpace being bigger than glyph bitmap data
	   width.
	*/
	
	TextExtent(rp, string, len, &te);
	SetDrMd(rp, old_drmd ^ INVERSVID);
	RectFill(rp, rp->cp_x + te.te_Extent.MinX,
	    	     rp->cp_y + te.te_Extent.MinY,
		     rp->cp_x + te.te_Extent.MaxX,
		     rp->cp_y + te.te_Extent.MaxY);
	SetDrMd(rp, old_drmd);
    }
    
    /* does this rastport have a layer. If yes, lock the layer it.*/
    if (NULL != rp->Layer)
      LockLayerRom(rp->Layer);	
    
    tf = rp->Font;
    
    if (ExtendFont(tf, NULL))
    {
    	fontbm = TFE_INTERN(tf->tf_Extension)->hash->font_bitmap;
    }
    
    /* Check if font has character data as a HIDD bitmap */
    
    if (NULL == fontbm)
    {
    	D(bug("FONT HAS NO HIDD BITMAP ! Won't render text\n"));
	RELEASE_DRIVERDATA(rp, GfxBase);
	return;
    }


    /* Render along font's baseline */
    render_y = rp->cp_y - tf->tf_Baseline;
    current_x = rp->cp_x;
    
    while ( len -- )
    {
	ULONG charloc;
	ULONG idx;
	
	if (*string < tf->tf_LoChar || *string > tf->tf_HiChar )
	{
	     /* A character which there is no glyph for. We just
	        draw the last glyph in the font
	     */
	     idx = NUMCHARS(tf) - 1;
	}
	else
	{
	    idx = *string - tf->tf_LoChar;
	}
	
	charloc = ((ULONG *)tf->tf_CharLoc)[idx];
	
	if (tf->tf_CharKern)
	{
    	    current_x += ((WORD *)tf->tf_CharKern)[idx];
	}
	    
	if ((tf->tf_Style & FSF_COLORFONT) &&
	    !(CTF(tf)->ctf_Flags & CT_ANTIALIAS))
	{
    	    #warning Handle color fonts	
	}
	else
	{
	       WORD
	    glyphXOffset = charloc >> 16,
        glyphWidth = charloc & 0xFFFF;

	    ASSERT(tf->tf_YSize > 0);

/* blit the glypth if it has data in the bitmap */
        if (glyphWidth > 0)
        {
            blit_glyph_fast(rp, fontbm, glyphXOffset, current_x, render_y,
                glyphWidth, tf->tf_YSize, GfxBase);
        }
	}
	
	if (tf->tf_CharSpace)
	    current_x += ((WORD *)tf->tf_CharSpace)[idx];
	else
	    current_x += tf->tf_XSize; /* Add glyph width */
	
	current_x += rp->TxSpacing;
	
	string ++;
    } /* for (each character to render) */
    
    Move(rp, current_x, rp->cp_y);
    
    if (NULL != rp->Layer)
      UnlockLayerRom(rp->Layer);
    
    RELEASE_DRIVERDATA(rp, GfxBase);
    
    return;

}

/***********************************************/
/* CYBERGFX CALLS                            ***/


struct wpa_render_data
{
    UBYTE *array;
    HIDDT_StdPixFmt pixfmt;
    ULONG modulo;
    ULONG bppix;
};

static ULONG wpa_render(APTR wpar_data
	, LONG srcx, LONG srcy
	, OOP_Object *dstbm_obj
	, OOP_Object *dst_gc
	, LONG x1, LONG y1, LONG x2, LONG y2
	, struct GfxBase *GfxBase)
{
    struct wpa_render_data *wpard;
    ULONG width, height;
    UBYTE *array;
    
    width  = x2 - x1 + 1;
    height = y2 - y1 + 1;
    
    wpard = (struct wpa_render_data *)wpar_data;
    
    array = wpard->array + wpard->modulo * srcy + wpard->bppix * srcx;
    
    HIDD_BM_PutImage(dstbm_obj
    	, dst_gc, array
	, wpard->modulo
	, x1, y1
	, width, height
	, wpard->pixfmt
    );
    
    return width * height;
}

struct wpaa_render_data
{
    UBYTE *array;
    ULONG modulo;
};

static ULONG wpaa_render(APTR wpaar_data
	, LONG srcx, LONG srcy
	, OOP_Object *dstbm_obj
	, OOP_Object *dst_gc
	, LONG x1, LONG y1, LONG x2, LONG y2
	, struct GfxBase *GfxBase)
{
    struct wpaa_render_data *wpaard;
    ULONG   	    	     width, height;
    UBYTE   	    	    *array;
    
    width  = x2 - x1 + 1;
    height = y2 - y1 + 1;
    
    wpaard = (struct wpaa_render_data *)wpaar_data;
    
    array = wpaard->array + wpaard->modulo * srcy + 4 * srcx;
    
    HIDD_BM_PutAlphaImage(dstbm_obj
    	, dst_gc, array
	, wpaard->modulo
	, x1, y1
	, width, height
    );
    
    return width * height;
}

struct bta_render_data
{
    UBYTE *array;
    ULONG  modulo;
    UBYTE  invertalpha;
};

static ULONG bta_render(APTR bta_data
	, LONG srcx, LONG srcy
	, OOP_Object *dstbm_obj
	, OOP_Object *dst_gc
	, LONG x1, LONG y1, LONG x2, LONG y2
	, struct GfxBase *GfxBase)
{
    struct bta_render_data *btard;
    ULONG   	    	    width, height;
    UBYTE   	    	   *array;
    
    width  = x2 - x1 + 1;
    height = y2 - y1 + 1;
    
    btard = (struct bta_render_data *)bta_data;
    
    array = btard->array + btard->modulo * srcy + srcx;
    
    HIDD_BM_PutAlphaTemplate(dstbm_obj
    	, dst_gc, array
	, btard->modulo
	, x1, y1
	, width, height
	, btard->invertalpha
    );
    
    return width * height;
}

struct rpa_render_data {
    UBYTE *array;
    HIDDT_StdPixFmt pixfmt;
    ULONG modulo;
    ULONG bppix;
};

static ULONG rpa_render(APTR rpar_data
	, LONG srcx, LONG srcy
	, OOP_Object *dstbm_obj
	, OOP_Object *dst_gc
	, LONG x1, LONG y1, LONG x2, LONG y2
	, struct GfxBase *GfxBase)
{
    struct rpa_render_data *rpard;
    ULONG width, height;
    UBYTE *array;
    
    width  = x2 - x1 + 1;
    height = y2 - y1 + 1;
    
    rpard = (struct rpa_render_data *)rpar_data;
    
    array = rpard->array + rpard->modulo * srcy + rpard->bppix * srcx;
    
    HIDD_BM_GetImage(dstbm_obj
    	, array
	, rpard->modulo
	, x1, y1
	, width, height
	, rpard->pixfmt
    );
    
    return width * height;
}

static LONG pix_read(APTR pr_data
	, OOP_Object *bm, OOP_Object *gc
	, LONG x, LONG y
	, struct GfxBase *GfxBase)
{
    struct rgbpix_render_data *prd;
    
    prd = (struct rgbpix_render_data *)pr_data;
    
    prd->pixel = HIDD_BM_GetPixel(bm, x, y);

    
    return 0;
}    


struct extcol_render_data {
    struct render_special_info rsi;
    struct BitMap *destbm;
    HIDDT_Pixel pixel;
    
};

static VOID buf_to_extcol(struct extcol_render_data *ecrd
	, LONG srcx, LONG srcy
	, LONG dstx, LONG dsty
	, ULONG width, ULONG height
	, HIDDT_Pixel *pixbuf
	, OOP_Object *bm_obj
	, HIDDT_Pixel *pixtab)
{
    LONG y;
    struct BitMap *bm;
    bm = ecrd->destbm;
    for (y = 0; y < height; y ++) {
    	LONG x;
	
    	for (x = 0; x < width; x ++) {
	    if (*pixbuf ++ == ecrd->pixel) {
	    	
	    	UBYTE *plane;
		ULONG i;
	    	/* Set the according bit in the bitmap */
		for (i = 0; i < bm->Depth; i ++) {
		    plane = bm->Planes[i];
		    if (NULL != plane) {
		    	UBYTE mask;
			
			plane += COORD_TO_BYTEIDX(x + dstx, y + dsty, bm->BytesPerRow);
			mask = XCOORD_TO_MASK(x + dstx);
			
			/* Set the pixel */
			*plane |= mask;
		    
		    } /* if (plane allocated) */
		} /* for (plane) */
	    } /* if (color match) */
	} /* for (x) */
    } /* for (y) */
    
    return;
}
	
	

static ULONG extcol_render(APTR funcdata
	, LONG dstx, LONG dsty
	, OOP_Object *dstbm_obj
	, OOP_Object *dst_gc
	, LONG x1, LONG y1, LONG x2, LONG y2
	, struct GfxBase *GfxBase)
{
    /* Get the info from the hidd */
    struct extcol_render_data *ecrd;
     
    ecrd = (struct extcol_render_data *)funcdata;
     
    hidd2buf_fast(ecrd->rsi.curbm
     	, x1, y1
	, (APTR)ecrd
	, dstx, dsty
	, x2 - x1 + 1
	, y2 - y1 + 1
	, buf_to_extcol
	, GfxBase
    );
		
    return (x2 - x1 + 1) * (y2 - y1 + 1);
}


struct dm_message {
    APTR memptr;
    ULONG offsetx;
    ULONG offsety;
    ULONG xsize;
    ULONG ysize;
    UWORD bytesperrow;
    UWORD bytesperpix;
    UWORD colormodel;
    
};

struct dm_render_data {
    struct dm_message msg;
    OOP_Object *pf;
    struct Hook *hook;
    struct RastPort *rp;
    IPTR stdpf;
    OOP_Object *gc;
};


static ULONG dm_render(APTR dmr_data
	, LONG srcx, LONG srcy
	, OOP_Object *dstbm_obj
	, OOP_Object *dst_gc
	, LONG x1, LONG y1, LONG x2, LONG y2
	, struct GfxBase *GfxBase)
{
    struct dm_render_data *dmrd;
    UBYTE *addr;
    struct dm_message *msg;
    IPTR bytesperpixel;
    ULONG width, height, fb_width, fb_height;
    ULONG banksize, memsize;
    
    dmrd = (struct dm_render_data *)dmr_data;
    width  = x2 - x1 + 1;
    height = y2 - y1 + 1;;
    msg = &dmrd->msg;
#if 1
    msg->offsetx = x1;
    msg->offsety = y1;
#else
    #warning "Not sure about this one . Set it to 0 since we adjust msg->memptr to x1/y1 lower down"
    msg->offsetx = 0; // x1;
    msg->offsety = 0; // y1;
#endif
    msg->xsize = width;
    msg->ysize = height;
    
    /* Get the baseadress from where to render */
    if (HIDD_BM_ObtainDirectAccess(dstbm_obj
    	, &addr
	, &fb_height, &fb_width
	, &banksize, &memsize)) {

	OOP_GetAttr(dmrd->pf, aHidd_PixFmt_BytesPerPixel, &bytesperpixel);
	msg->bytesperpix = (UWORD)bytesperpixel;
    
	/* Colormodel allready set */
    
	/* Compute the adress for the start pixel */
	#warning "We should maybe use something else than the BytesPerLine method since we may have alignment"
	msg->bytesperrow = HIDD_BM_BytesPerLine(dstbm_obj, dmrd->stdpf, width);
#if 1
	msg->memptr = addr;
#else
	msg->memptr = addr + (msg->bytesperrow * y1) + (bytesperpixel * x1);
#endif
	
	HIDD_BM_ReleaseDirectAccess(dstbm_obj);
	
	CallHookPkt(dmrd->hook, dmrd->rp, msg);
	
    } else {
    	/* We are unable to gain direct access to the framebuffer,
	   so we have to emulate it
	*/
	ULONG bytesperrow;
	ULONG tocopy_h, max_tocopy_h;
	ULONG lines_todo;
    
	lines_todo = height;
    
	/* The HIDD bm does not have a base adress so we have to render into
	   it using a temporary buffer
	*/
   	OOP_GetAttr(dmrd->pf, aHidd_PixFmt_BytesPerPixel, &bytesperpixel);
	//bytesperrow = HIDD_BM_BytesPerLine(dstbm_obj, dmrd->stdpf, width);
	bytesperrow = width * bytesperpixel;


kprintf("width %d bytesperrow %d bytesperpixel %d\n", width, bytesperrow, bytesperpixel);
kprintf(" colormodel %d\n", msg->colormodel);

	if (PIXELBUF_SIZE < bytesperrow) {
	    D(bug("!!! NOT ENOUGH SPACE IN TEMP BUFFER FOR A SINGLE LINE IN DoCDrawMethodTagList() !!!\n"));
	    return 0;
    	}
    
    	/* Calculate number of lines we might copy */
    	max_tocopy_h = PIXELBUF_SIZE / bytesperrow;
    
    #if 1
    	msg->offsetx = 0;
	msg->offsety = 0;
    #endif
    	/* Get the maximum number of lines */
    	while (lines_todo != 0) {
	
            struct TagItem gc_tags[] = {
	    	{ aHidd_GC_DrawMode, vHidd_GC_DrawMode_Copy },
	    	{ TAG_DONE, 0UL }
	    };
	    
	    IPTR old_drmd;

    	    tocopy_h = MIN(lines_todo, max_tocopy_h);
    	    msg->memptr = PrivGBase(GfxBase)->pixel_buf;
	    msg->bytesperrow = bytesperrow;
    
	    msg->bytesperpix = (UWORD)bytesperpixel;

LOCK_PIXBUF
	    HIDD_BM_GetImage(dstbm_obj
		, (UBYTE *)PrivGBase(GfxBase)->pixel_buf
		, bytesperrow
		, x1, y1 + height - lines_todo, width, lines_todo
		, dmrd->stdpf
	    );
    	    
	    /* Use the hook to set some pixels */
	    CallHookPkt(dmrd->hook, dmrd->rp, msg);
	
	    OOP_GetAttr(dmrd->gc, aHidd_GC_DrawMode, &old_drmd);
	    OOP_SetAttrs(dmrd->gc, gc_tags);
	    HIDD_BM_PutImage(dstbm_obj, dmrd->gc
		, (UBYTE *)PrivGBase(GfxBase)->pixel_buf
		, bytesperrow
		, x1, y1 + height - lines_todo, width, lines_todo
		, dmrd->stdpf
	    );
	    gc_tags[0].ti_Data = (IPTR)old_drmd;
	    OOP_SetAttrs(dmrd->gc, gc_tags);
	
ULOCK_PIXBUF

    	    lines_todo -= tocopy_h;
	}

    }
    
    return width * height;
}



LONG driver_WriteLUTPixelArray(APTR srcrect,
	UWORD srcx, UWORD srcy,
	UWORD srcmod, struct RastPort *rp, APTR ctable,
	UWORD destx, UWORD desty,
	UWORD sizex, UWORD sizey,
	UBYTE ctabformat,
	struct GfxBase *GfxBase)
{
    ULONG depth;
    
    HIDDT_PixelLUT pixlut;
    HIDDT_Pixel pixtab[256];
    
    HIDDT_Color col;
    ULONG i;
    
    LONG pixwritten = 0;
    UBYTE *buf;
    
    /* This is cybergraphx. We only work wih HIDD bitmaps */
    if (!IS_HIDD_BM(rp->BitMap)) {
    	D(bug("!!!!! Trying to use CGFX call on non-hidd bitmap in WriteLUTPixelArray()!!!\n"));
    	return 0;
    }
    
    pixlut.entries	= 256;
    pixlut.pixels	= pixtab;
    
    depth = GetBitMapAttr(rp->BitMap, BMA_DEPTH);
    
    /* This call does only support bitmaps with depth > 8. Use WritePixelArray8
       for other bitmaps
    */
    
    if (depth <= 8) {
    	D(bug("!!! TRYING TO USE WriteLUTPixelArray() ON BITMAP WITH DEPTH < 8\n"));
    	return 0;
    }
	
    /* Curently only one format is supported */
    if (CTABFMT_XRGB8 != ctabformat) {
    	D(bug("!!! WriteLUTPixelArray() CALLED WITH UNSUPPORTED CTAB FORMAT %d\n"
		, ctabformat));
    	return 0;
    }
    col.alpha	= 0;
	
    /* Convert the coltab into native pixels */
    for (i = 0; i < 256; i ++) {
    	register ULONG rgb = ((ULONG *)ctable)[i];
    	col.red		= (HIDDT_ColComp)((rgb & 0x00FF0000) >> 8);
	col.green	= (HIDDT_ColComp)(rgb & 0x0000FF00);
	col.blue	= (HIDDT_ColComp)((rgb & 0x000000FF) << 8);
	
	pixtab[i] = HIDD_BM_MapColor(HIDD_BM_OBJ(rp->BitMap), &col);
    }
    
    buf = (UBYTE *)srcrect;
    
    buf += CHUNKY8_COORD_TO_BYTEIDX(srcx, srcy, srcmod);
    
    pixwritten = write_pixels_8(rp
    	, buf
	, srcmod
	, destx, desty
	, destx + sizex - 1, desty + sizey - 1
	, &pixlut
	, GfxBase
    );
    
    
    /* Now blit the colors onto the screen */
    
    return pixwritten;
}


LONG driver_WritePixelArray(APTR src, UWORD srcx, UWORD srcy
	, UWORD srcmod, struct RastPort *rp, UWORD destx, UWORD desty
	, UWORD width, UWORD height, UBYTE srcformat, struct GfxBase *GfxBase)
{
     
    OOP_Object *pf = 0;
    HIDDT_StdPixFmt srcfmt_hidd = 0, morphfmt_hidd = 0;
    ULONG start_offset;
    IPTR bppix;
    
    LONG pixwritten = 0;
    
    struct wpa_render_data wpard;
    struct Rectangle rr;

    if (RECTFMT_GREY8 == srcformat)
    {
    	static ULONG greytab[256];
	
	/* Ignore possible race conditions during
	   initialization. Have no bad effect. Just
	   double initializations. */
	   
	/* FIXME/KILLME: evil static array which goes into BSS section
	   which x86 native AROS regards as evil! */
	   
	if (greytab[255] == 0)
	{
	    WORD i;
	    
	    for(i = 0; i < 256; i++)
	    {
	    	greytab[i] = i * 0x010101;
	    }
	}
	
	return driver_WriteLUTPixelArray(src, srcx, srcy, srcmod,
	    	    	    	    	 rp, greytab, destx, desty,
					 width, height, CTABFMT_XRGB8,
					 GfxBase);
    }

    /* This is cybergraphx. We only work wih HIDD bitmaps */
    if (!IS_HIDD_BM(rp->BitMap))
    {
    	D(bug("!!!!! Trying to use CGFX call on non-hidd bitmap in WritePixelArray() !!!\n"));
    	return 0;
    }
    
    if (!OBTAIN_DRIVERDATA(rp, GfxBase))
	return 0;
	
    if (RECTFMT_LUT8 == srcformat)
    {
    
	HIDDT_PixelLUT pixlut = { 256, HIDD_BM_PIXTAB(rp->BitMap) };
	UBYTE * array = (UBYTE *)src;
	
	if (rp->BitMap->Flags & BMF_SPECIALFMT)
	{
	    RELEASE_DRIVERDATA(rp, GfxBase);
	    D(bug("!!! No CLUT in driver_WritePixelArray\n"));
	    return 0;
	}
	
	array += CHUNKY8_COORD_TO_BYTEIDX(srcx, srcy, srcmod);
	
    	pixwritten = write_pixels_8(rp
		, array, srcmod
		, destx, desty
		, destx + width - 1, desty + height - 1
		, &pixlut
		, GfxBase);
	
	RELEASE_DRIVERDATA(rp, GfxBase);
	
	return pixwritten;
    }
        
    switch (srcformat)
    {
    	case RECTFMT_RGB15  : srcfmt_hidd = vHidd_StdPixFmt_RGB15   ; break;
    	case RECTFMT_BGR15  : srcfmt_hidd = vHidd_StdPixFmt_BGR15   ; break;
    	case RECTFMT_RGB15PC: srcfmt_hidd = vHidd_StdPixFmt_RGB15_LE; break;
    	case RECTFMT_BGR15PC: srcfmt_hidd = vHidd_StdPixFmt_BGR15_LE; break;
    	case RECTFMT_RGB16  : srcfmt_hidd = vHidd_StdPixFmt_RGB16   ; break;
    	case RECTFMT_BGR16  : srcfmt_hidd = vHidd_StdPixFmt_BGR16   ; break;
    	case RECTFMT_RGB16PC: srcfmt_hidd = vHidd_StdPixFmt_RGB16_LE; break;
    	case RECTFMT_BGR16PC: srcfmt_hidd = vHidd_StdPixFmt_BGR16_LE; break;
	case RECTFMT_RGB24  : srcfmt_hidd = vHidd_StdPixFmt_RGB24   ; break;
    	case RECTFMT_BGR24  : srcfmt_hidd = vHidd_StdPixFmt_BGR24   ; break;
	case RECTFMT_ARGB32 : srcfmt_hidd = vHidd_StdPixFmt_ARGB32  ; morphfmt_hidd = vHidd_StdPixFmt_0RGB32; break;
    	case RECTFMT_BGRA32 : srcfmt_hidd = vHidd_StdPixFmt_BGRA32  ; morphfmt_hidd = vHidd_StdPixFmt_BGR032; break;
	case RECTFMT_RGBA32 : srcfmt_hidd = vHidd_StdPixFmt_RGBA32  ; morphfmt_hidd = vHidd_StdPixFmt_RGB032; break;
	case RECTFMT_ABGR32 : srcfmt_hidd = vHidd_StdPixFmt_ABGR32  ; morphfmt_hidd = vHidd_StdPixFmt_0BGR32; break;
	case RECTFMT_RAW  : srcfmt_hidd = vHidd_StdPixFmt_Native; break;
    }

    /* Compute the start of the array */

#warning Get rid of the below code ?
/* This can be done by passing the srcx and srcy parameters on to
   the HIDD bitmap and let it take care of it itself.
   This means that HIDD_BM_PutImage() gets a lot of parameters,
   which may not be necessary in real life.
   
   Compromise: convert from *CyberGfx* pixfmt to bppix using a table lookup.
   This is faster
*/
    if ((srcfmt_hidd == vHidd_StdPixFmt_Native) || (morphfmt_hidd != 0))
    {
    	OOP_GetAttr(HIDD_BM_OBJ(rp->BitMap), aHidd_BitMap_PixFmt, (IPTR *)&pf);
    }
    
    if (srcfmt_hidd != vHidd_StdPixFmt_Native)
    {
    	/* RECTFMT_ARGB32 on vHidd_StdPixFmt_0RGB32 bitmap ==> use vHidd_StdPixFmt_0RGB32 */
    	/* RECTFMT_BGRA32 on vHidd_StdPixFmt_BGR032 bitmap ==> use vHidd_StdPixFmt_BGR032 */
    	/* RECTFMT_RGBA32 on vHidd_StdPixFmt_RGB032 bitmap ==> use vHidd_StdPixFmt_RGB032 */
    	/* RECTFMT_ABGR32 on vHidd_StdPixFmt_0BGR32 bitmap ==> use vHidd_StdPixFmt_0BGR32 */
	
    	if (morphfmt_hidd != 0)
	{
	    IPTR stdpf;

	    OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, (IPTR *)&stdpf);	    
	    if (stdpf == morphfmt_hidd) srcfmt_hidd = morphfmt_hidd;
    }

    	pf = HIDD_Gfx_GetPixFmt(SDD(GfxBase)->gfxhidd, srcfmt_hidd);
    }
        
    OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, &bppix);
    
    start_offset = ((ULONG)srcy) * srcmod + srcx * bppix;
        
    wpard.array	 = ((UBYTE *)src) + start_offset;
    wpard.pixfmt = srcfmt_hidd;
    wpard.modulo = srcmod;
    wpard.bppix	 = bppix;
    
    rr.MinX = destx;
    rr.MinY = desty;
    rr.MaxX = destx + width  - 1;
    rr.MaxY = desty + height - 1;
    
    pixwritten = do_render_func(rp, NULL, &rr, wpa_render, &wpard, FALSE, GfxBase);

    RELEASE_DRIVERDATA(rp, GfxBase);  
      
    return pixwritten;
}

LONG driver_WritePixelArrayAlpha(APTR src, UWORD srcx, UWORD srcy
	, UWORD srcmod, struct RastPort *rp, UWORD destx, UWORD desty
	, UWORD width, UWORD height, ULONG globalalpha, struct GfxBase *GfxBase)
{
    ULONG   	    	    start_offset;    
    LONG    	    	    pixwritten = 0;    
    struct wpaa_render_data wpaard;
    struct Rectangle 	    rr;

    /* This is cybergraphx. We only work wih HIDD bitmaps */
    if (!IS_HIDD_BM(rp->BitMap))
    {
    	D(bug("!!!!! Trying to use CGFX call on non-hidd bitmap in WritePixelArrayAlpha() !!!\n"));
    	return 0;
    }
    
    if (!OBTAIN_DRIVERDATA(rp, GfxBase))
	return 0;
	
    /* Compute the start of the array */

    start_offset = ((ULONG)srcy) * srcmod + srcx * 4;
        
    wpaard.array  = ((UBYTE *)src) + start_offset;
    wpaard.modulo = srcmod;
    
    rr.MinX = destx;
    rr.MinY = desty;
    rr.MaxX = destx + width  - 1;
    rr.MaxY = desty + height - 1;
    
    pixwritten = do_render_func(rp, NULL, &rr, wpaa_render, &wpaard, FALSE, GfxBase);
    
    RELEASE_DRIVERDATA(rp, GfxBase);
    
    return pixwritten;
}

LONG driver_ReadPixelArray(APTR dst, UWORD destx, UWORD desty
	, UWORD dstmod, struct RastPort *rp, UWORD srcx, UWORD srcy
	, UWORD width, UWORD height, UBYTE dstformat, struct GfxBase *GfxBase)
{
     
    OOP_Object *pf = 0;    
    HIDDT_StdPixFmt dstfmt_hidd = 0, morphfmt_hidd = 0;
    
    ULONG start_offset;
    IPTR bppix;
    
    LONG pixread = 0;
    IPTR old_drmd;
    OOP_Object *gc;
    
    struct Rectangle rr;
    struct rpa_render_data rpard;

    struct TagItem gc_tags[] =
    {
	{ aHidd_GC_DrawMode , vHidd_GC_DrawMode_Copy},
	{ TAG_DONE  	          	    	    }
    };
    
    /* This is cybergraphx. We only work wih HIDD bitmaps */
    if (!IS_HIDD_BM(rp->BitMap))
    {
    	D(bug("!!!!! Trying to use CGFX call on non-hidd bitmap in ReadPixelArray() !!!\n"));
    	return 0;
    }
    
    if (!OBTAIN_DRIVERDATA(rp, GfxBase))
	return 0;
	
    gc = GetDriverData(rp)->dd_GC;

   /* Preserve old drawmode */
    OOP_GetAttr(gc, aHidd_GC_DrawMode, &old_drmd);
    OOP_SetAttrs(gc, gc_tags);
    
    
    switch (dstformat)
    {
    	case RECTFMT_RGB15  : dstfmt_hidd = vHidd_StdPixFmt_RGB15   ; break;
    	case RECTFMT_BGR15  : dstfmt_hidd = vHidd_StdPixFmt_BGR15   ; break;
    	case RECTFMT_RGB15PC: dstfmt_hidd = vHidd_StdPixFmt_RGB15_LE; break;
    	case RECTFMT_BGR15PC: dstfmt_hidd = vHidd_StdPixFmt_BGR15_LE; break;
    	case RECTFMT_RGB16  : dstfmt_hidd = vHidd_StdPixFmt_RGB16   ; break;
    	case RECTFMT_BGR16  : dstfmt_hidd = vHidd_StdPixFmt_BGR16   ; break;
    	case RECTFMT_RGB16PC: dstfmt_hidd = vHidd_StdPixFmt_RGB16_LE; break;
    	case RECTFMT_BGR16PC: dstfmt_hidd = vHidd_StdPixFmt_BGR16_LE; break;
	case RECTFMT_RGB24  : dstfmt_hidd = vHidd_StdPixFmt_RGB24   ; break;
    	case RECTFMT_BGR24  : dstfmt_hidd = vHidd_StdPixFmt_BGR24   ; break;
	case RECTFMT_ARGB32 : dstfmt_hidd = vHidd_StdPixFmt_ARGB32  ; morphfmt_hidd = vHidd_StdPixFmt_0RGB32; break;
    	case RECTFMT_BGRA32 : dstfmt_hidd = vHidd_StdPixFmt_BGRA32  ; morphfmt_hidd = vHidd_StdPixFmt_BGR032; break;
	case RECTFMT_RGBA32 : dstfmt_hidd = vHidd_StdPixFmt_RGBA32  ; morphfmt_hidd = vHidd_StdPixFmt_RGB032; break;
	case RECTFMT_ABGR32 : dstfmt_hidd = vHidd_StdPixFmt_ABGR32  ; morphfmt_hidd = vHidd_StdPixFmt_0BGR32; break;
	case RECTFMT_RAW  : dstfmt_hidd = vHidd_StdPixFmt_Native; break;
    }

#warning Get rid of the below code ?
/* This can be done by passing the srcx and srcy parameters on to
   the HIDD bitmap and let it take care of it itself.
   This means that HIDD_BM_PutImage() gets a lot of parameters,
   which may not be necessary in real life.
   
   Compromise: convert from *CyberGfx* pixfmt to bppix using a table lookup.
   This is faster
*/
    if ((dstfmt_hidd == vHidd_StdPixFmt_Native) || (morphfmt_hidd != 0))
    {
    	OOP_GetAttr(HIDD_BM_OBJ(rp->BitMap), aHidd_BitMap_PixFmt, (IPTR *)&pf);
    }
    
    if (dstfmt_hidd != vHidd_StdPixFmt_Native)
    {
    	/* RECTFMT_ARGB32 on vHidd_StdPixFmt_0RGB32 bitmap ==> use vHidd_StdPixFmt_0RGB32 */
    	/* RECTFMT_BGRA32 on vHidd_StdPixFmt_BGR032 bitmap ==> use vHidd_StdPixFmt_BGR032 */
    	/* RECTFMT_RGBA32 on vHidd_StdPixFmt_RGB032 bitmap ==> use vHidd_StdPixFmt_RGB032 */
    	/* RECTFMT_ABGR32 on vHidd_StdPixFmt_0BGR32 bitmap ==> use vHidd_StdPixFmt_0BGR32 */
	
	if (morphfmt_hidd != 0)
	{
	    IPTR stdpf;

	    OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, (IPTR *)&stdpf);	    
	    if (stdpf == morphfmt_hidd) dstfmt_hidd = morphfmt_hidd;
    }
       
    	pf = HIDD_Gfx_GetPixFmt(SDD(GfxBase)->gfxhidd, dstfmt_hidd);
    }
    
    OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, &bppix);
    
    start_offset = ((ULONG)desty) * dstmod + destx * bppix;
        
    rpard.array	 = ((UBYTE *)dst) + start_offset;
    rpard.pixfmt = dstfmt_hidd;
    rpard.modulo = dstmod;
    rpard.bppix	 = bppix;
    
    rr.MinX = srcx;
    rr.MinY = srcy;
    rr.MaxX = srcx + width  - 1;
    rr.MaxY = srcy + height - 1;
    
    pixread = do_render_func(rp, NULL, &rr, rpa_render, &rpard, FALSE, GfxBase);
    
    /* restore old gc values */
    gc_tags[0].ti_Data = (IPTR)old_drmd;
    OOP_SetAttrs(gc, gc_tags);

    RELEASE_DRIVERDATA(rp, GfxBase);

    return pixread;
}

LONG driver_InvertPixelArray(struct RastPort *rp
	, UWORD destx, UWORD desty, UWORD width, UWORD height
	, struct GfxBase *GfxBase)
{

    /* This is cybergraphx. We only work wih HIDD bitmaps */
    if (!IS_HIDD_BM(rp->BitMap))
    {
    	D(bug("!!!!! Trying to use CGFX call on non-hidd bitmap InvertPixelArray() !!!\n"));
    	return 0;
    }

    return (LONG)fillrect_pendrmd(rp
   	 , destx, desty
	 , destx + width  - 1
	 , desty + height - 1
	 , 0	/* Source pen does not matter */
	 , vHidd_GC_DrawMode_Invert
	 , GfxBase);
}

LONG driver_FillPixelArray(struct RastPort *rp
	, UWORD destx, UWORD desty, UWORD width, UWORD height
	, ULONG pixel, struct GfxBase *GfxBase) 
{
    HIDDT_Color col;
    HIDDT_Pixel pix;
    
    /* HIDDT_ColComp are 16 Bit */
    col.alpha	= (HIDDT_ColComp)((pixel >> 16) & 0x0000FF00);
    col.red	= (HIDDT_ColComp)((pixel >> 8) & 0x0000FF00);
    col.green	= (HIDDT_ColComp)(pixel & 0x0000FF00);
    col.blue	= (HIDDT_ColComp)((pixel << 8) & 0x0000FF00);
    
    pix = HIDD_BM_MapColor(HIDD_BM_OBJ(rp->BitMap), &col);

    return (LONG)fillrect_pendrmd(rp
	, destx, desty
	, destx + width  - 1
	, desty + height - 1
	, pix
	, vHidd_GC_DrawMode_Copy
	, GfxBase
    );
}

ULONG driver_MovePixelArray(UWORD srcx, UWORD srcy, struct RastPort *rp
	, UWORD destx, UWORD desty, UWORD width, UWORD height
	, struct GfxBase *GfxBase)
{
    ClipBlit(rp
		, srcx, srcy
		, rp
		, destx, desty
		, width, height
		, 0x00C0 /* Copy */
    );
    return width * height;
}



LONG driver_WriteRGBPixel(struct RastPort *rp, UWORD x, UWORD y
	, ULONG pixel, struct GfxBase *GfxBase)
{
    
    struct rgbpix_render_data  prd;
    HIDDT_Color     	    col;
    LONG    	    	    retval;
    
    /* This is cybergraphx. We only work wih HIDD bitmaps */
    if (!IS_HIDD_BM(rp->BitMap))
    {
    	D(bug("!!!!! Trying to use CGFX call on non-hidd bitmap in WriteRGBPixel() !!!\n"));
    	return 0;
    }

    if (!OBTAIN_DRIVERDATA(rp, GfxBase)) return -1;
    
    /* HIDDT_ColComp are 16 Bit */
    
    col.alpha	= (HIDDT_ColComp)((pixel >> 16) & 0x0000FF00);
    col.red	= (HIDDT_ColComp)((pixel >> 8) & 0x0000FF00);
    col.green	= (HIDDT_ColComp)(pixel & 0x0000FF00);
    col.blue	= (HIDDT_ColComp)((pixel << 8) & 0x0000FF00);
    
    prd.pixel = HIDD_BM_MapColor(HIDD_BM_OBJ(rp->BitMap), &col);
    
    retval = do_pixel_func(rp, x, y, rgbpix_write, &prd, GfxBase);
      
    RELEASE_DRIVERDATA(rp, GfxBase);
    
    return retval;
   
}


ULONG driver_ReadRGBPixel(struct RastPort *rp, UWORD x, UWORD y
	, struct GfxBase *GfxBase)
{
    struct rgbpix_render_data prd;
    
    /* Get the HIDD pixel val */
    HIDDT_Color col;
    HIDDT_Pixel pix;
    LONG ret;
    
    /* This is cybergraphx. We only work wih HIDD bitmaps */
    if (!IS_HIDD_BM(rp->BitMap))
    {
    	D(bug("!!!!! Trying to use CGFX call on non-hidd bitmap in ReadRGBPixel()!!!\n"));
    	return (ULONG)-1;
    }
    
    if (!OBTAIN_DRIVERDATA(rp, GfxBase)) return (ULONG)-1;
    
    ret = do_pixel_func(rp, x, y, pix_read, &prd, GfxBase);
    
    RELEASE_DRIVERDATA(rp, GfxBase);
    
    if (-1 == ret)
    	return (ULONG)-1;

    HIDD_BM_UnmapPixel(HIDD_BM_OBJ(rp->BitMap), prd.pixel, &col);

    /* HIDDT_ColComp are 16 Bit */
    
    pix =	  ((col.alpha & 0xFF00) << 16)
    		| ((col.red & 0xFF00) << 8)
		| (col.green & 0xFF00)
		| ((col.blue & 0xFF00) >> 8);
    
    return pix;
}



ULONG driver_GetCyberMapAttr(struct BitMap *bitMap, ULONG attribute, struct GfxBase *GfxBase)
{
    OOP_Object *bm_obj;
    OOP_Object *pf;
    
    IPTR retval;
    
    /* This is cybergraphx. We only work wih HIDD bitmaps */
    if (!IS_HIDD_BM(bitMap)) {
    	D(bug("!!!!! Trying to use CGFX call on non-hidd bitmap in GetCyberMapAttr() !!!\n"));
    	return 0;
    }
	
    bm_obj = HIDD_BM_OBJ(bitMap);
    
    OOP_GetAttr(bm_obj, aHidd_BitMap_PixFmt, (IPTR *)&pf);
    
    switch (attribute) {
   	case CYBRMATTR_XMOD:
	    OOP_GetAttr(bm_obj, aHidd_BitMap_BytesPerRow, &retval);
	    break;
	
   	case CYBRMATTR_BPPIX:
	    OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, &retval);
	    break;
	
   	case CYBRMATTR_PIXFMT: {
	    IPTR stdpf;
	    UWORD cpf;
	    OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, (IPTR *)&stdpf);
	    
	    /* Convert to cybergfx */
	    cpf = hidd2cyber_pixfmt(stdpf, GfxBase);
	    
	    if (cpf == (UWORD)-1) {
	    	D(bug("!!! UNKNOWN PIXEL FORMAT IN GetCyberMapAttr()\n"));
	    }
	    
	    retval = (IPTR)cpf;
	    break;
	    
	}
	
   	case CYBRMATTR_WIDTH:
	#if 0 /* stegerg: doesn't really work, because of framebuffer bitmap object stuff */
	    OOP_GetAttr(bm_obj, aHidd_BitMap_Width, &retval);
	#else
	    retval = GetBitMapAttr(bitMap, BMA_WIDTH);
	#endif
	    break;
	
   	case CYBRMATTR_HEIGHT:
	#if 0 /* stegerg: doesn't really work, because of framebuffer bitmap object stuff */
	    OOP_GetAttr(bm_obj, aHidd_BitMap_Height, &retval);
	#else
	    retval = GetBitMapAttr(bitMap, BMA_HEIGHT);
	#endif
	    break;
	
   	case CYBRMATTR_DEPTH:
	#if 0 /* stegerg: might not really work, because of framebuffer bitmap object stuff */
	    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &retval);
	#else
	    retval = GetBitMapAttr(bitMap, BMA_DEPTH);
	#endif
	    break;
	
   	case CYBRMATTR_ISCYBERGFX: {
	    IPTR depth;
	    
	    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
	    
	    if (depth < 8) {
	    	retval = 0;
	    } else {
	    /* We allways have a HIDD bitmap */
	    	retval = 0xFFFFFFFF; /* Some apps seem to rely on this retval */
	    }
	    break; }
	
   	case CYBRMATTR_ISLINEARMEM:
	    OOP_GetAttr(bm_obj, aHidd_BitMap_IsLinearMem, &retval);
	    break;
	
	default:
	    D(bug("!!! UNKNOWN ATTRIBUTE PASSED TO GetCyberMapAttr()\n"));
	    break;
	
	
    } /* switch (attribute) */
    
    return retval;
}


VOID driver_CVideoCtrlTagList(struct ViewPort *vp, struct TagItem *tags, struct GfxBase *GfxBase)
{
    struct TagItem *tag, *tstate;
    ULONG dpmslevel = 0;
    
    struct TagItem htags[] =
    {
	{ aHidd_Gfx_DPMSLevel,	0UL	},
	{ TAG_DONE, 0UL }    
    };
    
    BOOL dpms_found = FALSE;
    
    HIDDT_DPMSLevel hdpms = 0;
    
    for (tstate = tags; (tag = NextTagItem((const struct TagItem **)&tstate)); )
    {
    	switch (tag->ti_Tag)
	{
	    case SETVC_DPMSLevel:
	    	dpmslevel = tag->ti_Data;
		dpms_found = TRUE;
	    	break;
	    
	    default:
	    	D(bug("!!! UNKNOWN TAG IN CVideoCtrlTagList(): %x !!!\n"
			, tag->ti_Tag));
		break;
	    
	} /* switch() */
	
    } /* for (each tagitem) */
    
   
    if (dpms_found)
    {  
    
	/* Convert to hidd dpms level */
	switch (dpmslevel)
	{
	    case DPMS_ON:
	    	hdpms = vHidd_Gfx_DPMSLevel_On;
	    	break;

	    case DPMS_STANDBY:
	    	hdpms = vHidd_Gfx_DPMSLevel_Standby;
	    	break;

	    case DPMS_SUSPEND:
	    	hdpms = vHidd_Gfx_DPMSLevel_Suspend;
	    	break;

	    case DPMS_OFF:
	    	hdpms = vHidd_Gfx_DPMSLevel_Off;
	    	break;
	
	    default:
	    	D(bug("!!! UNKNOWN DPMS LEVEL IN CVideoCtrlTagList(): %x !!!\n"
	    	    , dpmslevel));
		    
		dpms_found = FALSE;
		break;
	
	}
    }
    
    if (dpms_found)
    {
	htags[0].ti_Data = hdpms;
    }
    else
    {
    	htags[0].ti_Tag = TAG_IGNORE;
    }
    
    OOP_SetAttrs(SDD(GfxBase)->gfxhidd, htags);
    
    return;
}


ULONG driver_ExtractColor(struct RastPort *rp, struct BitMap *bm
	, ULONG color, ULONG srcx, ULONG srcy, ULONG width, ULONG height
	, struct GfxBase *GfxBase)
{
    struct Rectangle rr;
    LONG pixread = 0;
    struct extcol_render_data ecrd;
    OOP_Object *pf;
    IPTR colmod;
    
    if (!IS_HIDD_BM(rp->BitMap))
    {
    	D(bug("!!! CALLING ExtractColor() ON NO-HIDD BITMAP !!!\n"));
	return FALSE;
    }

    if (!OBTAIN_DRIVERDATA(rp, GfxBase))
    	return FALSE;
	    
    rr.MinX = srcx;
    rr.MinY = srcy;
    rr.MaxX = srcx + width  - 1;
    rr.MaxY = srcy + height - 1;
    
    OOP_GetAttr(HIDD_BM_OBJ(rp->BitMap), aHidd_BitMap_PixFmt, (IPTR *)&pf);
    
    OOP_GetAttr(pf, aHidd_PixFmt_ColorModel, (IPTR *)&colmod);
    
    if (vHidd_ColorModel_Palette == colmod)
    {
        ecrd.pixel = color;
    }
    else
    {
	HIDDT_Color col;
	
	col.alpha = (color >> 16) & 0x0000FF00;
	col.red	  = (color >> 8 ) & 0x0000FF00;
	col.green = color & 0x0000FF00;
	col.blue  = (color << 8) & 0x0000FF00;
	
	ecrd.pixel = HIDD_BM_MapColor(HIDD_BM_OBJ(rp->BitMap), &col);
    
    }
    
    ecrd.destbm = bm;
    
    pixread = do_render_func(rp, NULL, &rr, extcol_render, NULL, TRUE, GfxBase);
	
    RELEASE_DRIVERDATA(rp, GfxBase);
    
    if (pixread != (width * height))
    	return FALSE;
	
    return TRUE;
}


VOID driver_DoCDrawMethodTagList(struct Hook *hook, struct RastPort *rp, struct TagItem *tags, struct GfxBase *GfxBase)
{

    struct dm_render_data dmrd;
    struct Rectangle rr;
    struct Layer *L;
    
    if (!IS_HIDD_BM(rp->BitMap))
    {
    	D(bug("!!! NO HIDD BITMAP IN CALL TO DoCDrawMethodTagList() !!!\n"));
	return;
    }

    if (!OBTAIN_DRIVERDATA(rp, GfxBase))
    	return;
	
    /* Get the bitmap std pixfmt */    
    OOP_GetAttr(HIDD_BM_OBJ(rp->BitMap), aHidd_BitMap_PixFmt, (IPTR *)&dmrd.pf);
    OOP_GetAttr(dmrd.pf, aHidd_PixFmt_StdPixFmt, &dmrd.stdpf);
    dmrd.msg.colormodel = hidd2cyber_pixfmt(dmrd.stdpf, GfxBase);
    dmrd.hook = hook;
    dmrd.rp = rp;
    
    if (((UWORD)-1) == dmrd.msg.colormodel)
    {
    	RELEASE_DRIVERDATA(rp, GfxBase);
    	D(bug("!!! UNKNOWN HIDD PIXFMT IN DoCDrawMethodTagList() !!!\n"));
	return;
    }
    
    
    L = rp->Layer;

    rr.MinX = 0;
    rr.MinY = 0;
    
    if (NULL == L)
    {
	rr.MaxX = GetBitMapAttr(rp->BitMap, BMA_WIDTH)  - 1;
	rr.MaxY = GetBitMapAttr(rp->BitMap, BMA_HEIGHT) - 1;
    }
    else
    {
    	/* Lock the layer */
	LockLayerRom(L);
    
    	rr.MaxX = rr.MinX + (L->bounds.MaxX - L->bounds.MinX) - 1;
	rr.MaxY = rr.MinY + (L->bounds.MaxY - L->bounds.MinY) - 1;
    }
    
    dmrd.gc = GetDriverData(rp)->dd_GC;
    do_render_func(rp, NULL, &rr, dm_render, &dmrd, FALSE, GfxBase);
    
    RELEASE_DRIVERDATA(rp, GfxBase);
    
    if (NULL != L)
    {
	UnlockLayerRom(L);
    }
    
    return;
}

APTR driver_LockBitMapTagList(struct BitMap *bm, struct TagItem *tags, struct GfxBase *GfxBase)
{
    struct TagItem *tag;
    UBYTE *baseaddress;
    ULONG width, height, banksize, memsize;
    OOP_Object *pf;
    IPTR stdpf;
    UWORD cpf;
    
    if (!IS_HIDD_BM(bm))
    {
    	D(bug("!!! TRYING TO CALL LockBitMapTagList() ON NON-HIDD BM !!!\n"));
	return NULL;
    }

    OOP_GetAttr(HIDD_BM_OBJ(bm), aHidd_BitMap_PixFmt, (IPTR *)&pf);
    
    OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, &stdpf);
    cpf = hidd2cyber_pixfmt(stdpf, GfxBase);
    if (((UWORD)-1) == cpf)
    {
    	D(bug("!!! TRYING TO CALL LockBitMapTagList() ON NON-CYBER PIXFMT BITMAP !!!\n"));
	return NULL;
    }
    
    /* Get some info from the bitmap object */
    if (!HIDD_BM_ObtainDirectAccess(HIDD_BM_OBJ(bm), &baseaddress, &width, &height, &banksize, &memsize))
    	return NULL;
    
    
    while ((tag = NextTagItem((const struct TagItem **)&tags)))
    {
    	switch (tag->ti_Tag)
	{
	    case LBMI_BASEADDRESS:
	    	*((IPTR **)tag->ti_Data) = (IPTR *)baseaddress;
	    	break;
		
	    case LBMI_BYTESPERROW:
	    	*((IPTR *)tag->ti_Data) = 
			(ULONG)HIDD_BM_BytesPerLine(HIDD_BM_OBJ(bm), stdpf, width);
	    	break;
	    
	    case LBMI_BYTESPERPIX:
	    	OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, (IPTR *)tag->ti_Data);
	    	break;
	    
	    case LBMI_PIXFMT: 
		*((IPTR *)tag->ti_Data) = (IPTR)cpf;
	    	break;
		
	    case LBMI_DEPTH:
	    	OOP_GetAttr(pf, aHidd_PixFmt_Depth, (IPTR *)tag->ti_Data);
		break;
	    
	    case LBMI_WIDTH:
	    	OOP_GetAttr(HIDD_BM_OBJ(bm), aHidd_BitMap_Width, (IPTR *)tag->ti_Data);
	    	break;
	    
	    case LBMI_HEIGHT:
	    	OOP_GetAttr(HIDD_BM_OBJ(bm), aHidd_BitMap_Height, (IPTR *)tag->ti_Data);
	    	break;
		
	    default:
	    	D(bug("!!! UNKNOWN TAG PASSED TO LockBitMapTagList() !!!\n"));
		break;
	}
    }
    
    return HIDD_BM_OBJ(bm);
}

VOID driver_UnLockBitMap(APTR handle, struct GfxBase *GfxBase)
{
    if (handle) HIDD_BM_ReleaseDirectAccess((OOP_Object *)handle);
}

VOID driver_UnLockBitMapTagList(APTR handle, struct TagItem *tags, struct GfxBase *GfxBase)
{
    struct TagItem *tag;
    BOOL reallyunlock = TRUE;
    
    while ((tag = NextTagItem((const struct TagItem **)&tags)))
    {
    	switch (tag->ti_Tag)
	{
	    case UBMI_REALLYUNLOCK:
	    	reallyunlock = (BOOL)tag->ti_Data;
		break;
		
	    case UBMI_UPDATERECTS:
	    {
	    	struct RectList *rl;
		
		rl = (struct RectList *)tag->ti_Data;
		
#warning Dunno what to do with this
		
	    	break;
	    }
	
	    default:
	    	D(bug("!!! UNKNOWN TAG PASSED TO UnLockBitMapTagList() !!!\n"));
		break;
	}
    }
    
    if (reallyunlock)
    {
	HIDD_BM_ReleaseDirectAccess((OOP_Object *)handle);
    }
}

void driver_BltTemplateAlpha(UBYTE *src, LONG srcx, LONG srcmod
    	, struct RastPort *rp, LONG destx, LONG desty, LONG width, LONG height
	, struct GfxBase *GfxBase)
{
    struct bta_render_data  btard;
    struct Rectangle 	    rr;

    /* This is cybergraphx. We only work wih HIDD bitmaps */
    if (!IS_HIDD_BM(rp->BitMap)) {
    	D(bug("!!!!! Trying to use CGFX call on non-hidd bitmap in BltTemplateAlpha() !!!\n"));
    	return;
    }
    
    if (!OBTAIN_DRIVERDATA(rp, GfxBase))
	return;
	
    /* Compute the start of the array */

    btard.array  = src + srcx;
    btard.modulo = srcmod;
    btard.invertalpha = (rp->DrawMode & INVERSVID) ? TRUE : FALSE;
    rr.MinX = destx;
    rr.MinY = desty;
    rr.MaxX = destx + width  - 1;
    rr.MaxY = desty + height - 1;
    
    do_render_func(rp, NULL, &rr, bta_render, &btard, FALSE, GfxBase);
    
    RELEASE_DRIVERDATA(rp, GfxBase);
}

/******************************************/
/* Support stuff for cybergfx             */
/******************************************/

#undef GfxBase

