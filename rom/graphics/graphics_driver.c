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

#include "fakegfxhidd.h"
#include "graphics_intern.h"
#include "intregions.h"
#include "dispinfo.h"
#include "gfxfuncsupport.h"
#include "fontsupport.h"

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

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
    if (SDD(GfxBase)->framebuffer) {
	OOP_DisposeObject(SDD(GfxBase)->framebuffer);
	SDD(GfxBase)->framebuffer = NULL;
    }

#if 0
    if (SDD(GfxBase)->activescreen_inited)
	cleanup_activescreen_stuff(GfxBase);
#endif
    if (SDD(GfxBase)->dispinfo_db) {
	destroy_dispinfo_db(SDD(GfxBase)->dispinfo_db, GfxBase);
	SDD(GfxBase)->dispinfo_db = NULL;
    }

    if ( SDD(GfxBase)->planarbm_cache ) {
	delete_object_cache( SDD(GfxBase)->planarbm_cache, GfxBase );
	SDD(GfxBase)->planarbm_cache = NULL;
    }

    if ( SDD(GfxBase)->gc_cache ) {
	delete_object_cache( SDD(GfxBase)->gc_cache, GfxBase );
	SDD(GfxBase)->gc_cache = NULL;
    }

    if ( SDD(GfxBase)->fakegfx_inited ) {
	cleanup_fakegfxhidd( &SDD(GfxBase)->fakegfx_staticdata, GfxBase);
	SDD(GfxBase)->fakegfx_inited = FALSE;
    }

    if ( SDD(GfxBase)->gfxhidd_orig ) {
	OOP_DisposeObject( SDD(GfxBase)->gfxhidd_orig );
	SDD(GfxBase)->gfxhidd_orig = NULL;
    }
     
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

BOOL driver_LateGfxInit (APTR data, struct GfxBase *GfxBase)
{

    /* Supplied data is really the librarybase of a HIDD */
    STRPTR gfxhiddname = (STRPTR)data;
    struct TagItem tags[] = {
    	{ TAG_DONE, 0UL },
    };
    EnterFunc(bug("driver_LateGfxInit(gfxhiddname=%s)\n", gfxhiddname));

    driver_expunge(GfxBase);
    D(bug("[GFX] Cleanup complete\n"));

    /* Create a new GfxHidd object */

    SDD(GfxBase)->gfxhidd = SDD(GfxBase)->gfxhidd_orig = OOP_NewObject(NULL, gfxhiddname, tags);
    D(bug("driver_LateGfxInit: gfxhidd=%p\n", SDD(GfxBase)->gfxhidd));

    if (NULL != SDD(GfxBase)->gfxhidd) {
	IPTR hwcursor = 0;
	IPTR noframebuffer = 0;
	BOOL ok = TRUE;

    	/* Get method id here, to avoid any possible semaphore involment when calling the method */ 
    	SDD(GfxBase)->hiddGfxShowImminentReset_MethodID = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_ShowImminentReset);

	OOP_GetAttr(SDD(GfxBase)->gfxhidd, aHidd_Gfx_SupportsHWCursor, &hwcursor);
	OOP_GetAttr(SDD(GfxBase)->gfxhidd, aHidd_Gfx_NoFrameBuffer, &noframebuffer);
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
		        if (!noframebuffer)
			    SDD(GfxBase)->framebuffer = create_framebuffer(GfxBase);
			if (noframebuffer || SDD(GfxBase)->framebuffer) {
			    D(bug("FRAMEBUFFER OK: %p\n", SDD(GfxBase)->framebuffer));
		            ReturnBool("driver_LateGfxInit", TRUE);
			}
			if (SDD(GfxBase)->framebuffer)
			    OOP_DisposeObject(SDD(GfxBase)->framebuffer);
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
	
    do_render_func(rp, NULL, &rr, bgf_render, &bgfrd, TRUE, FALSE, GfxBase);
	
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
    	fontbm = ((struct TextFontExtension_intern *)(tf->tf_Extension))->hash->font_bitmap;
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

/******************************************/
/* Support stuff for cybergfx             */
/******************************************/

ULONG DoRenderFunc(struct RastPort *rp, Point *src, struct Rectangle *rr,
			 ULONG (*render_func)(APTR, LONG, LONG, OOP_Object *, OOP_Object *, LONG, LONG, LONG, LONG, struct GfxBase *),
			 APTR funcdata, BOOL do_update, struct GfxBase *GfxBase)
{
    ULONG res;

    if (!OBTAIN_DRIVERDATA(rp, GfxBase))
        return -1;

    res = do_render_func(rp, src, rr, render_func, funcdata, do_update, FALSE, GfxBase);

    RELEASE_DRIVERDATA(rp, GfxBase);
    return res;
}

ULONG DoPixelFunc(struct RastPort *rp, LONG x, LONG y,
    	    	    LONG (*render_func)(APTR, OOP_Object *, OOP_Object *, LONG, LONG, struct GfxBase *),
		    APTR funcdata, BOOL do_update, struct GfxBase *GfxBase)
{
    ULONG res;

    if (!OBTAIN_DRIVERDATA(rp, GfxBase))
        return -1;

    res = do_pixel_func(rp, x, y, render_func, funcdata, do_update, GfxBase);

    RELEASE_DRIVERDATA(rp, GfxBase);
    return res;
}

OOP_Object *GetDriverGC(struct RastPort *rp, struct GfxBase *GfxBase)
{
    if (!OBTAIN_DRIVERDATA(rp, GfxBase))
        return NULL;

    return GetDriverData(rp)->dd_GC;
}
