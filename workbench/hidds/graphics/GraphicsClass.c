/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics hidd class implementation.
    Lang: english
*/

/****************************************************************************************/

#include <aros/config.h>
#include <aros/symbolsets.h>
#include <exec/lists.h>

#include "graphics_intern.h"

#include <string.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <exec/libraries.h>
#include <exec/memory.h>

#include <utility/tagitem.h>
#include <hidd/graphics.h>

#include LC_LIBDEFS_FILE

#undef  SDEBUG
#undef  DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/

static BOOL create_std_pixfmts(struct class_static_data *_csd);
static VOID delete_std_pixfmts(struct class_static_data *_csd);
static VOID free_objectlist(struct List *list, BOOL OOP_DisposeObjects,
    	    	    	    struct class_static_data *_csd);
static BOOL register_modes(OOP_Class *cl, OOP_Object *o, struct TagItem *modetags);

static BOOL alloc_mode_db(struct mode_db *mdb, ULONG numsyncs, ULONG numpfs, OOP_Class *cl);
static VOID free_mode_db(struct mode_db *mdb, OOP_Class *cl);
static OOP_Object *create_and_init_object(OOP_Class *cl, UBYTE *data, ULONG datasize,
    	    	    	    	    	  struct class_static_data *_csd);

static struct pfnode *find_pixfmt(struct MinList *pflist
	, HIDDT_PixelFormat *tofind
	, struct class_static_data *_csd);

static OOP_Object *find_stdpixfmt(HIDDT_PixelFormat *tofind
	, struct class_static_data *_csd);
static VOID copy_bm_and_colmap(OOP_Class *cl, OOP_Object *o,  OOP_Object *src_bm
	, OOP_Object *dst_bm, OOP_Object *dims_bm);

BOOL parse_pixfmt_tags(struct TagItem *tags, HIDDT_PixelFormat *pf, ULONG attrcheck, struct class_static_data *_csd);
BOOL parse_sync_tags(struct TagItem *tags, struct sync_data *data, ULONG attrcheck, struct class_static_data *_csd);

/****************************************************************************************/

#define COMPUTE_HIDD_MODEID(sync, pf)	\
    ( ((sync) << 16) | (pf) )
    
#define MODEID_TO_SYNCIDX(id) ( (id) >> 16 )
#define MODEID_TO_PFIDX(id) ( (id) & 0x0000FFFF )

/****************************************************************************************/

OOP_Object *GFX__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct HIDDGraphicsData *data;
    BOOL    	    	    ok = FALSE;
    struct TagItem  	    *modetags;
    struct TagItem  	    gctags[] =
    {
    	{TAG_DONE, 0UL}
    };

    D(bug("Entering gfx.hidd::New\n"));
    
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (NULL == o)
    	return NULL;
    
    D(bug("Got object o=%x\n", o));

    data = OOP_INST_DATA(cl, o);
    
    NEWLIST(&data->pflist);
    
    InitSemaphore(&data->mdb.sema);
    InitSemaphore(&data->pfsema);
    data->curmode = vHidd_ModeID_Invalid;
    
    /* Get the mode tags */
    modetags = (struct TagItem *)GetTagData(aHidd_Gfx_ModeTags, NULL, msg->attrList);
    if (NULL != modetags)
    {
	/* Parse it and register the gfxmodes */
	if (register_modes(cl, o, modetags))
	{
	    ok = TRUE;
	}
	else
	    D(bug("Could not register modes\n"));
    }
    else
	D(bug("Could not get ModeTags\n"));
    
    /* Create a gc that we can use for some rendering */
    if (ok)
    {
	data->gc = OOP_NewObject(CSD(cl)->gcclass, NULL, gctags);
	if (NULL == data->gc)
	{
	    D(bug("Could not get gc\n"));
	    ok = FALSE;
	}
    }
    
    if (!ok)
    {
	D(bug("Not OK\n"));
	OOP_MethodID dispose_mid;
	
	dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);
	o = NULL;
    }
    
    D(bug("Leaving gfx.hidd::New o=%x\n", o));

    return o;
}

/****************************************************************************************/

VOID GFX__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct HIDDGraphicsData *data;
    
    data = OOP_INST_DATA(cl, o);
    
    /* free the mode db stuff */
    free_mode_db(&data->mdb, cl);
      
    ObtainSemaphore(&data->pfsema);
    free_objectlist((struct List *)&data->pflist, TRUE, CSD(cl));
    ReleaseSemaphore(&data->pfsema);
    
    if (NULL != data->gc)
	OOP_DisposeObject(data->gc);

    OOP_DoSuperMethod(cl, o, msg);
}

/****************************************************************************************/

VOID GFX__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct HIDDGraphicsData *data;
    BOOL    	    	    found = FALSE;
    ULONG   	    	    idx;
    
    data = OOP_INST_DATA(cl, o);
    
    if (IS_GFX_ATTR(msg->attrID, idx))
    {
	switch (idx)
	{
	    case aoHidd_Gfx_NumSyncs:
	    {
		found = TRUE;
		*msg->storage = data->mdb.num_syncs;
		break;
	    }
	    
	    case aoHidd_Gfx_SupportsHWCursor:
	    	found = TRUE;
	    	*msg->storage = FALSE;
		break;

	    case aoHidd_Gfx_IsWindowed:
	    	found = TRUE;
	    	*msg->storage = FALSE;
		break;
	    
	    default:	/* Keep compiler happy */
		break;
	}
    }
    
    if (!found)
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	
    return;    
}

/****************************************************************************************/

OOP_Object *GFX__Hidd_Gfx__NewGC(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NewGC *msg)
{
    OOP_Object *gc = NULL;

    EnterFunc(bug("HIDDGfx::NewGC()\n"));

    gc = OOP_NewObject(NULL, CLID_Hidd_GC, msg->attrList);

    ReturnPtr("HIDDGfx::NewGC", OOP_Object *, gc);
}

/****************************************************************************************/

VOID GFX__Hidd_Gfx__DisposeGC(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_DisposeGC *msg)
{
    EnterFunc(bug("HIDDGfx::DisposeGC()\n"));

    if (NULL != msg->gc) OOP_DisposeObject(msg->gc);

    ReturnVoid("HIDDGfx::DisposeGC");
}

/****************************************************************************************/

#define BMAO(x) aoHidd_BitMap_ ## x
#define BMAF(x) (1L << aoHidd_BitMap_ ## x)

#define BM_DIMS_AF  (BMAF(Width) | BMAF(Height))

#define SET_TAG(tags, idx, tag, val)	\
    tags[idx].ti_Tag = tag ; tags[idx].ti_Data = (IPTR)val;

#define SET_BM_TAG(tags, idx, tag, val)	\
    SET_TAG(tags, idx, aHidd_BitMap_ ## tag, val)

/****************************************************************************************/
	
OOP_Object * GFX__Hidd_Gfx__NewBitMap(OOP_Class *cl, OOP_Object *o,
    	    	    	    	      struct pHidd_Gfx_NewBitMap *msg)
{
    struct TagItem  	    bmtags[7];
    
    IPTR    	    	    attrs[num_Total_BitMap_Attrs];
    STRPTR  	    	    classid = NULL;
    OOP_Class 	    	    *classptr = NULL;
    BOOL    	    	    displayable = FALSE; /* Default attr value */
    BOOL    	    	    framebuffer = FALSE;
    OOP_Object      	    *pf = NULL, *sync;
    APTR		    ptr_pf = &pf;
    HIDDT_ModeID    	    modeid = 0;
    OOP_Object      	    *bm;
    struct HIDDGraphicsData *data;
    
    DECLARE_ATTRCHECK(bitmap);
    
    BOOL    	    	    gotclass = FALSE;

    data = OOP_INST_DATA(cl, o);
    
    if (0 != OOP_ParseAttrs(msg->attrList, attrs, num_Total_BitMap_Attrs,
    	    	    	    &ATTRCHECK(bitmap), HiddBitMapAttrBase))
    {
	D(bug("!!! FAILED TO PARSE ATTRS IN Gfx::NewBitMap !!!\n"));
	return NULL;
    }
    
    if (GOT_BM_ATTR(PixFmt))
    {
	D(bug("!!! Gfx::NewBitMap: USER IS NOT ALLOWED TO PASS aHidd_BitMap_PixFmt !!!\n"));
	return NULL;
    }
    
    /* Get class supplied by superclass */
    if (GOT_BM_ATTR(ClassPtr))
    {
    	classptr	= (OOP_Class *)attrs[BMAO(ClassPtr)];
	gotclass = TRUE;
    }
    else
    {
	if (GOT_BM_ATTR(ClassID))
	{
    	    classid	= (STRPTR)attrs[BMAO(ClassID)];
	    gotclass = TRUE;
	}
    }
		
    if (GOT_BM_ATTR(Displayable))
	displayable = (BOOL)attrs[BMAO(Displayable)];

    if (GOT_BM_ATTR(FrameBuffer))
    {
    	framebuffer = (BOOL)attrs[BMAO(FrameBuffer)];
	if (framebuffer) displayable = TRUE;
    }

    if (GOT_BM_ATTR(ModeID))
    {
	modeid = attrs[BMAO(ModeID)];
	
	/* Check that it is a valid mode */
	if (!HIDD_Gfx_GetMode(o, modeid, &sync, &pf))
	{
	    D(bug("!!! Gfx::NewBitMap: USER PASSED INVALID MODEID !!!\n"));
	}
    }

    /* First argument is gfxhidd */    
    SET_BM_TAG(bmtags, 0, GfxHidd, o);
    SET_BM_TAG(bmtags, 1, Displayable, displayable);
    
	
    if (displayable || framebuffer)
    {
	/* The user has to supply a modeid */
	if (!GOT_BM_ATTR(ModeID))
	{
	    D(bug("!!! Gfx::NewBitMap: USER HAS NOT PASSED MODEID FOR DISPLAYABLE BITMAP !!!\n"));
	    return NULL;
	}
	
	if (!gotclass)
	{
	    D(bug("!!! Gfx::NewBitMap: SUBCLASS DID NOT PASS CLASS FOR DISPLAYABLE BITMAP !!!\n"));
	    return NULL;
	}
	
	SET_BM_TAG(bmtags, 2, ModeID, modeid);
	SET_BM_TAG(bmtags, 3, PixFmt, pf);
	
	if (framebuffer)
	{
	    SET_BM_TAG(bmtags, 4, FrameBuffer, TRUE);
	}
	else
	{
	    SET_TAG(bmtags, 4, TAG_IGNORE, 0UL);
	}
	SET_TAG(bmtags, 5, TAG_MORE, msg->attrList);
	
    }
    else
    { /* if (displayable) */
	ULONG width, height;
    
	/* To get a pixfmt for an offscreen bitmap we either need 
	    (ModeID || ( (Width && Height) && StdPixFmt) || ( (Width && Height) && Friend))
	*/
	    
	if (GOT_BM_ATTR(ModeID))
	{   
	    /* We have allredy gotten pixelformat and sync for the modeid case */
	    OOP_GetAttr(sync, aHidd_Sync_HDisp, &width);
	    OOP_GetAttr(sync, aHidd_Sync_VDisp, &height);
	}
	else
	{
	    /* Next to look for is StdPixFmt */
	    
	    /* Check that we have width && height */
	    if (BM_DIMS_AF != (BM_DIMS_AF & ATTRCHECK(bitmap)))
	    {
		D(bug("!!! Gfx::NewBitMap() MISSING WIDTH/HEIGHT TAGS !!!\n"));
		return NULL;
	    }
	    
	    width  = attrs[BMAO(Width)];
	    height = attrs[BMAO(Height)];
	    	    
	    if (GOT_BM_ATTR(StdPixFmt))
	    {
		pf = HIDD_Gfx_GetPixFmt(o, (HIDDT_StdPixFmt)attrs[BMAO(StdPixFmt)]);
		if (NULL == pf)
		{
		    D(bug("!!! Gfx::NewBitMap(): USER PASSED BOGUS StdPixFmt !!!\n"));
		    return NULL;
		}
	    }
	    else
	    {
		/* Last alternative is that the user passed a friend bitmap */
		if (GOT_BM_ATTR(Friend))
		{
		    OOP_GetAttr((OOP_Object *)attrs[BMAO(Friend)], aHidd_BitMap_PixFmt, (IPTR *)ptr_pf);
		}
		else
		{
		    D(bug("!!! Gfx::NewBitMap: UNSIFFICIENT ATTRS TO CREATE OFFSCREEN BITMAP !!!\n"));
		    return NULL;
		}
	    }
	}
	
	/* Did the subclass provide an offbitmap class for us ? */
	if (!gotclass)
	{
	    /* Have to find a suitable class ourselves */
	    HIDDT_BitMapType bmtype;
		
	    OOP_GetAttr(pf, aHidd_PixFmt_BitMapType, &bmtype);
	    switch (bmtype)
	    {
	        case vHidd_BitMapType_Chunky:
		    classptr = CSD(cl)->chunkybmclass;
		    break;
		    
	        case vHidd_BitMapType_Planar:
		    classptr = CSD(cl)->planarbmclass;
		    break;
		    
	        default:
	    	    D(bug("!!! Gfx::NewBitMap: UNKNOWN BITMAPTYPE %d !!!\n", bmtype));
		    return NULL;
		    
	    }
	    
	} /* if (!gotclass) */
	
	/* Set the tags we want to pass to the selected bitmap class */
	SET_BM_TAG(bmtags, 2, Width,  width);
	SET_BM_TAG(bmtags, 3, Height, height);
	SET_BM_TAG(bmtags, 4, PixFmt, pf);

	if (GOT_BM_ATTR(Friend))
	{
	    SET_BM_TAG(bmtags, 5, Friend, attrs[BMAO(Friend)]);
	}
	else
	{
	    SET_TAG(bmtags, 5, TAG_IGNORE, 0UL);
	}
	SET_TAG(bmtags, 6, TAG_MORE, msg->attrList);
	
    } /* if (!displayable) */
    

    bm = OOP_NewObject(classptr, classid, bmtags);

    if (framebuffer)
    	data->framebuffer = bm;
	
    return bm;
    
}

/****************************************************************************************/

VOID GFX__Hidd_Gfx__DisposeBitMap(OOP_Class *cl, OOP_Object *o,
				  struct pHidd_Gfx_DisposeBitMap *msg)
{
    if (NULL != msg->bitMap)
	OOP_DisposeObject(msg->bitMap);
}

/****************************************************************************************/

#define SD(x) 	    	    	    ((struct sync_data *)x)
#define PF(x) 	    	    	    ((HIDDT_PixelFormat *)x)

#define XCOORD_TO_BYTEIDX(x) 	    ( (x) >> 3)
#define COORD_TO_BYTEIDX(x, y, bpr) ( ( (y) * bpr ) + XCOORD_TO_BYTEIDX(x) )
#define XCOORD_TO_MASK(x)   	    (1L << (7 - ((x) & 0x07) ))
#define WIDTH_TO_BYTES(width) 	    ( (( (width) - 1) >> 3) + 1)

/****************************************************************************************/

/* modebm functions pfidx is x and syncidx is y coord in the bitmap */

/****************************************************************************************/

static inline BOOL alloc_mode_bm(struct mode_bm *bm, ULONG numsyncs, ULONG numpfs,
    	    	    	    	 OOP_Class *cl)
{
    bm->bpr = WIDTH_TO_BYTES(numpfs);
    
    bm->bm = AllocVec(bm->bpr * numsyncs, MEMF_CLEAR);
    if (NULL == bm->bm)
	return FALSE;
	
    /* We initialize the mode bitmap to all modes valid */
    memset(bm->bm, 0xFF, bm->bpr * numsyncs);
    
    return TRUE;
}

/****************************************************************************************/

static inline VOID free_mode_bm(struct mode_bm *bm, OOP_Class *cl)
{
    FreeVec(bm->bm);
    bm->bm  = NULL;
    bm->bpr = 0;
}

/****************************************************************************************/

static inline BOOL is_valid_mode(struct mode_bm *bm, ULONG syncidx, ULONG pfidx)
{
    if (0 != (XCOORD_TO_MASK(pfidx) & bm->bm[COORD_TO_BYTEIDX(pfidx, syncidx, bm->bpr)]))
	return TRUE;
	
    return FALSE;
}

/****************************************************************************************/

static inline VOID set_valid_mode(struct mode_bm *bm, ULONG syncidx, ULONG pfidx,
    	    	    	    	  BOOL valid)
{
    if (valid)
	bm->bm[COORD_TO_BYTEIDX(pfidx, syncidx, bm->bpr)] |= XCOORD_TO_MASK(pfidx);
    else
	bm->bm[COORD_TO_BYTEIDX(pfidx, syncidx, bm->bpr)] &= ~XCOORD_TO_MASK(pfidx);

    return;
}

/****************************************************************************************/

static BOOL alloc_mode_db(struct mode_db *mdb, ULONG numsyncs, ULONG numpfs, OOP_Class *cl)
{
    BOOL ok = FALSE;
    
    if (0 == numsyncs || 0 == numpfs)
	return FALSE;
	
    ObtainSemaphore(&mdb->sema);
    /* free_mode_bm() needs this */
    mdb->num_pixfmts	= numpfs;
    mdb->num_syncs	= numsyncs;

    mdb->syncs	 = AllocMem(sizeof (OOP_Object *) * numsyncs, MEMF_CLEAR);
    
    if (NULL != mdb->syncs)
    {
	mdb->pixfmts = AllocMem(sizeof (OOP_Object *) * numpfs,   MEMF_CLEAR);
	
	if (NULL != mdb->pixfmts)
	{
	    if (alloc_mode_bm(&mdb->orig_mode_bm, numsyncs, numpfs, cl))
	    {
		if (alloc_mode_bm(&mdb->checked_mode_bm, numsyncs, numpfs, cl))
		{
		    ok = TRUE;
		}
	    }
	}
    }
    
    if (!ok)
	free_mode_db(mdb, cl);
	
    ReleaseSemaphore(&mdb->sema);
    
    return ok;
}

/****************************************************************************************/

static VOID free_mode_db(struct mode_db *mdb, OOP_Class *cl)
{
    ULONG i;
    
    ObtainSemaphore(&mdb->sema);
    
    if (NULL != mdb->pixfmts)
    {
    
/****** !!! NB !!! The Pixel formats are registerd in the
  GfxMode Database and is freed in the
  free_object_list functions

    	for (i = 0; i < mdb->num_pixfmts; i ++)
	{
	    if (NULL != mdb->pixfmts[i])
	    {
	    	OOP_DisposeObject(mdb->pixfmts[i]);
		mdb->pixfmts[i] = NULL;
	    }
	}
*/

	FreeMem(mdb->pixfmts, sizeof (OOP_Object *) * mdb->num_pixfmts);
	mdb->pixfmts = NULL; mdb->num_pixfmts = 0;
    }

    if (NULL != mdb->syncs)
    {
    	for (i = 0; i < mdb->num_syncs; i ++)
	{
	    if (NULL != mdb->syncs[i])
	    {
	    
	    	OOP_DisposeObject(mdb->syncs[i]);
		mdb->syncs[i] = NULL;
	    }
	}
	
	FreeMem(mdb->syncs, sizeof (OOP_Object *) * mdb->num_syncs);
	mdb->syncs = NULL; mdb->num_syncs = 0;
    }
    
    if (NULL != mdb->orig_mode_bm.bm)
    {
	free_mode_bm(&mdb->orig_mode_bm, cl);
    }
    
    if (NULL != mdb->checked_mode_bm.bm)
    {
	free_mode_bm(&mdb->checked_mode_bm, cl);
    }
    
    ReleaseSemaphore(&mdb->sema);
    
    return;
}

/****************************************************************************************/

/* Initializes default tagarray. in numtags the TAG_MORE is not accounted for,
   so the array must be of size NUM_TAGS + 1
*/

/****************************************************************************************/

static VOID init_def_tags(struct TagItem *tags, ULONG numtags)
{
    ULONG i;
    
    for (i = 0; i < numtags; i ++)
    {
	tags[i].ti_Tag = TAG_IGNORE;
	tags[i].ti_Data = 0UL;
    }
    
    tags[i].ti_Tag  = TAG_MORE;
    tags[i].ti_Data = 0UL;
    
    return;
}

/****************************************************************************************/

static BOOL register_modes(OOP_Class *cl, OOP_Object *o, struct TagItem *modetags)
{
    struct TagItem  	    *tag, *tstate;
    struct HIDDGraphicsData *data;
    
    DECLARE_ATTRCHECK(sync);
    
    struct mode_db  	    *mdb;
    
    HIDDT_PixelFormat 	    pixfmt_data;
    struct sync_data 	    sync_data;
    
    struct TagItem  	    def_sync_tags[num_Hidd_Sync_Attrs     + 1];
    struct TagItem  	    def_pixfmt_tags[num_Hidd_PixFmt_Attrs + 1];
    
    ULONG   	    	    numpfs = 0,numsyncs	= 0;
    ULONG   	    	    pfidx = 0, syncidx = 0;
    
    data = OOP_INST_DATA(cl, o);
    mdb = &data->mdb;
    InitSemaphore(&mdb->sema);
    
    memset(&pixfmt_data, 0, sizeof (pixfmt_data));
    memset(&sync_data,   0, sizeof (sync_data));
    
    init_def_tags(def_sync_tags,	num_Hidd_Sync_Attrs);
    init_def_tags(def_pixfmt_tags,	num_Hidd_PixFmt_Attrs);
    
    /* First we need to calculate how much memory we are to allocate by counting supplied
       pixel formats and syncs */
    
    for (tstate = modetags; (tag = NextTagItem((const struct TagItem **)&tstate));)
    {
	ULONG idx;
	
	if (IS_GFX_ATTR(tag->ti_Tag, idx))
	{
	    switch (idx)
	    {
		case aoHidd_Gfx_PixFmtTags:
		    numpfs++;
		    break;
		    
		case aoHidd_Gfx_SyncTags:
		    numsyncs ++;
		    break;
		    
		default:
		    break;
	    }
	}
    }
    
    if (0 == numpfs || 0 == numsyncs)
    {
	D(bug("!!! WE MUST AT LEAST HAVE ONE PIXFMT AND ONE SYNC IN Gfx::RegisterModes() !!!\n"));
    }

    ObtainSemaphore(&mdb->sema);
    
    /* Allocate memory for mode db */
    if (!alloc_mode_db(&data->mdb, numsyncs, numpfs, cl))
	goto failure;
    
    
    for (tstate = modetags; (tag = NextTagItem((const struct TagItem **)&tstate));)
    {
	/* Look for Gfx, PixFmt and Sync tags */
	ULONG idx;
	
	if (IS_GFX_ATTR(tag->ti_Tag, idx))
	{
	    switch (idx)
	    {
		case aoHidd_Gfx_PixFmtTags:
		    def_pixfmt_tags[num_Hidd_PixFmt_Attrs].ti_Data = tag->ti_Data;
		    mdb->pixfmts[pfidx] = HIDD_Gfx_RegisterPixFmt(o, def_pixfmt_tags);
		    
		    if (NULL == mdb->pixfmts[pfidx])
		    {
			D(bug("!!! UNABLE TO CREATE PIXFMT OBJECT IN Gfx::RegisterModes() !!!\n"));
			goto failure;
		    }
		    
		    pfidx ++;
		    break;
		    
		case aoHidd_Gfx_SyncTags:
		    def_sync_tags[num_Hidd_Sync_Attrs].ti_Data = tag->ti_Data;
		    if (!parse_sync_tags(def_sync_tags
			    , &sync_data
			    , ATTRCHECK(sync)
			    , CSD(cl) ))
		    {
			D(bug("!!! ERROR PARSING SYNC TAGS IN Gfx::RegisterModes() !!!\n"));
			goto failure;
		    }
		    else
		    {
			mdb->syncs[syncidx] = create_and_init_object(CSD(cl)->syncclass
			    , (UBYTE *)&sync_data
			    , sizeof (sync_data)
			    , CSD(cl) );
			    
			if (NULL == mdb->syncs[syncidx])
			{
			    D(bug("!!! UNABLE TO CREATE PIXFMT OBJECT IN Gfx::RegisterModes() !!!\n"));
			    goto failure;
			}
			syncidx ++;
		    }
		    break;
	    }
	    
	}
	else if (IS_SYNC_ATTR(tag->ti_Tag, idx))
	{
	    if (idx >= num_Hidd_Sync_Attrs)
	    {
		D(bug("!!! UNKNOWN SYNC ATTR IN Gfx::New(): %d !!!\n", idx));
	    }
	    else
	    {
		def_sync_tags[idx].ti_Tag  = tag->ti_Tag;
		def_sync_tags[idx].ti_Data = tag->ti_Data;
	    }
	    
	}
	else if (IS_PIXFMT_ATTR(tag->ti_Tag, idx))
	{
	    if (idx >= num_Hidd_PixFmt_Attrs)
	    {
		D(bug("!!! UNKNOWN PIXFMT ATTR IN Gfx::New(): %d !!!\n", idx));
	    }
	    else
	    {
		def_pixfmt_tags[idx].ti_Tag  = tag->ti_Tag;
		def_pixfmt_tags[idx].ti_Data = tag->ti_Data;
	    }
	}
    }
    
    ReleaseSemaphore(&mdb->sema);
    
    return TRUE;
    
failure:

    /*	mode db is freed in dispose */
    ReleaseSemaphore(&mdb->sema);
    
    return FALSE;
}

/****************************************************************************************/

struct modequery
{
    struct mode_db  *mdb;
    ULONG   	    minwidth;
    ULONG   	    maxwidth;
    ULONG   	    minheight;
    ULONG   	    maxheight;
    HIDDT_StdPixFmt *stdpfs;
    ULONG   	    numfound;
    ULONG   	    pfidx;
    ULONG   	    syncidx;
    BOOL    	    dims_ok;
    BOOL    	    stdpfs_ok;
    BOOL    	    check_ok;
    OOP_Class 	    *cl;
};

/****************************************************************************************/

/* This is a recursive function that looks for valid modes */

/****************************************************************************************/

static HIDDT_ModeID *querymode(struct modequery *mq)
{
    HIDDT_ModeID    	*modeids;
    register OOP_Object *pf;
    register OOP_Object *sync;
    BOOL    	    	mode_ok = FALSE;
    OOP_Class 	    	*cl = mq->cl;
    ULONG   	    	syncidx, pfidx;
    
    mq->dims_ok	  = FALSE;
    mq->stdpfs_ok = FALSE;
    mq->check_ok  = FALSE;
    
    /* Look at the supplied idx */
    if (mq->pfidx >= mq->mdb->num_pixfmts)
    {
	mq->pfidx = 0;
	mq->syncidx ++;
    }

    if (mq->syncidx >= mq->mdb->num_syncs)
    {
	/* We have reached the end of the recursion. Allocate memory and go back 
	*/
	
	modeids = AllocVec(sizeof (HIDDT_ModeID) * (mq->numfound + 1), MEMF_ANY);
	/* Get the end of the array */
	modeids += mq->numfound;
	*modeids = vHidd_ModeID_Invalid;
	
	return modeids;
    }

    syncidx = mq->syncidx;
    pfidx   = mq->pfidx;
    /* Get the pf and sync objects */
    pf   = mq->mdb->pixfmts[syncidx];
    sync = mq->mdb->syncs[pfidx];
    

    /* Check that the mode is really usable */
    if (is_valid_mode(&mq->mdb->checked_mode_bm, syncidx, pfidx))
    {
	mq->check_ok = TRUE;
    
    
	/* See if this mode matches the criterias set */
	
	if (	SD(sync)->hdisp  >= mq->minwidth
	     && SD(sync)->hdisp  <= mq->maxwidth
	     && SD(sync)->vdisp >= mq->minheight
	     && SD(sync)->vdisp <= mq->maxheight	)
	{
	     
	     
	    mq->dims_ok = TRUE;

	    if (NULL != mq->stdpfs)
	    {
		register HIDDT_StdPixFmt *stdpf = mq->stdpfs;
		while (*stdpf)
		{
		    if (*stdpf == PF(pf)->stdpixfmt)
		    {
		       	mq->stdpfs_ok  = TRUE;
		    }
		    stdpf ++;
		}
	    }
	    else
	    {
	    	mq->stdpfs_ok = TRUE;
	    }
	}
    }
    
    
    if (mq->dims_ok && mq->stdpfs_ok && mq->check_ok)
    {
	mode_ok = TRUE;
	mq->numfound ++;
    }
    
    mq->pfidx ++;
    
    modeids = querymode(mq);

    if (NULL == modeids)
	return NULL;
	
    if (mode_ok)
    {
	/* The mode is OK. Add it to the list */
	modeids --;
	*modeids = COMPUTE_HIDD_MODEID(syncidx, pfidx);
    }
    
    return modeids;
	
}

/****************************************************************************************/

HIDDT_ModeID *GFX__Hidd_Gfx__QueryModeIDs(OOP_Class *cl, OOP_Object *o,
    	    	    	    	    	  struct pHidd_Gfx_QueryModeIDs *msg)
{
    struct TagItem  	    *tag, *tstate;
    
    HIDDT_ModeID    	    *modeids;
    struct HIDDGraphicsData *data;
    struct mode_db  	    *mdb;
    
    struct modequery 	    mq =
    {
	NULL,		/* mode db (set later)	*/
	0, 0xFFFFFFFF, 	/* minwidth, maxwidth	*/
	0, 0xFFFFFFFF,	/* minheight, maxheight	*/
	NULL,		/* stdpfs		*/
	0,		/* numfound		*/
	0, 0,		/* pfidx, syncidx	*/
	FALSE, FALSE,	/* dims_ok, stdpfs_ok	*/
	FALSE,		/* check_ok		*/
	NULL		/* class (set later)	*/
	
    };
    

    data = OOP_INST_DATA(cl, o);
    mdb = &data->mdb;
    mq.mdb = mdb;
    mq.cl  = cl;
    
    for (tstate = msg->queryTags; (tag = NextTagItem((const struct TagItem **)&tstate)); )
    {
	switch (tag->ti_Tag)
	{
	    case tHidd_GfxMode_MinWidth:
	    	mq.minwidth = (ULONG)tag->ti_Tag;
		break;

	    case tHidd_GfxMode_MaxWidth:
	    	mq.maxwidth = (ULONG)tag->ti_Tag;
		break;

	    case tHidd_GfxMode_MinHeight:
	    	mq.minheight = (ULONG)tag->ti_Tag;
		break;

	    case tHidd_GfxMode_MaxHeight:
	    	mq.maxheight = (ULONG)tag->ti_Tag;
		break;
		
	    case tHidd_GfxMode_PixFmts:
	    	mq.stdpfs = (HIDDT_StdPixFmt *)tag->ti_Data;
		break;

	}
    }

    ObtainSemaphoreShared(&mdb->sema);

    /* Recursively check all modes */    
    modeids = querymode(&mq);
    
    ReleaseSemaphore(&mdb->sema);
    
    return modeids;
     
}

/****************************************************************************************/

VOID GFX__Hidd_Gfx__ReleaseModeIDs(OOP_Class *cl, OOP_Object *o,
    	    	    	    	   struct pHidd_Gfx_ReleaseModeIDs *msg)
{
    FreeVec(msg->modeIDs);
}

/****************************************************************************************/

HIDDT_ModeID GFX__Hidd_Gfx__NextModeID(OOP_Class *cl, OOP_Object *o,
    	    	    	    	       struct pHidd_Gfx_NextModeID *msg)
{
    struct HIDDGraphicsData *data;
    struct mode_db  	    *mdb;
    ULONG   	    	    syncidx, pfidx;
    HIDDT_ModeID    	    return_id = vHidd_ModeID_Invalid;
    BOOL    	    	    found = FALSE;
    
    data = OOP_INST_DATA(cl, o);
    mdb = &data->mdb;

    ObtainSemaphoreShared(&mdb->sema);    
    if (vHidd_ModeID_Invalid == msg->modeID)
    {
	pfidx	= 0;
	syncidx = 0;	
    }
    else
    {
	pfidx 	= MODEID_TO_PFIDX( msg->modeID );
	syncidx	= MODEID_TO_SYNCIDX( msg->modeID );

	/* Increament one from the last call */
	pfidx ++;
	if (pfidx >= mdb->num_pixfmts)
	{
	    pfidx = 0;
	    syncidx ++;
	}
    }
    
    /* Search for a new mode. We only accept valid modes */
    for (; syncidx < mdb->num_syncs; syncidx ++)
    {
	/* We only return valid modes */
	for (; pfidx < mdb->num_pixfmts; pfidx ++)
	{
	    if (is_valid_mode(&mdb->checked_mode_bm, syncidx, pfidx))
	    {
		found = TRUE;
		break;
	    }
	}
	if (found)
	    break;
    }
    
    if (found)
    {
	return_id = COMPUTE_HIDD_MODEID(syncidx, pfidx);
	*msg->syncPtr	= mdb->syncs[syncidx];
	*msg->pixFmtPtr	= mdb->pixfmts[pfidx];
    }
    else
    {
	*msg->syncPtr = *msg->pixFmtPtr = NULL;
    }
	
    ReleaseSemaphore(&mdb->sema);
    
    return return_id;
}

/****************************************************************************************/

BOOL GFX__Hidd_Gfx__GetMode(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_GetMode *msg)
{
    ULONG   	    	    pfidx, syncidx;
    struct HIDDGraphicsData *data;
    struct mode_db  	    *mdb;
    BOOL    	    	    ok = FALSE;
    
    data = OOP_INST_DATA(cl, o);
    mdb = &data->mdb;
    
    pfidx	= MODEID_TO_PFIDX(msg->modeID);
    syncidx	= MODEID_TO_SYNCIDX(msg->modeID);
    
    ObtainSemaphoreShared(&mdb->sema);
    
    if (! (pfidx >= mdb->num_pixfmts || syncidx >= mdb->num_syncs) )
    {
	if (is_valid_mode(&mdb->checked_mode_bm, syncidx, pfidx))
	{
	    ok = TRUE;
	    *msg->syncPtr	= mdb->syncs[syncidx];
	    *msg->pixFmtPtr	= mdb->pixfmts[pfidx];
	}
    }
    
    ReleaseSemaphore(&mdb->sema);
    
    if (!ok)
    {
	*msg->syncPtr = *msg->pixFmtPtr = NULL;
    }
    
    return ok;
}

/****************************************************************************************/

BOOL GFX__Hidd_Gfx__SetMode(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetMode *msg)
{
#if 0    
    struct HIDDGraphicsData *data;
    OOP_Object      	    *sync, *pf;
    
    data = OOP_INST_DATA(cl, o);

    /* Check if we have a valid modeid */
    if (HIDD_Gfx_GetMode(o, msg->modeID, &sync, &pf))
    {

    	/* Mode exists. */
    	curmode = msg->modeID;
	
	/* Do we have a framebuffer yet ? */
	if (NULL == data->framebuffer)
	{
	    /* No framebuffer. We should init one. */
	}
	else
	{
	    /* Framebuffer created. Check if framebuffer is large enough
	    for mode. */
	    
	    
	} 
    }

#endif    
    return FALSE;
}

/****************************************************************************************/

static VOID copy_bm_and_colmap(OOP_Class *cl, OOP_Object *o,  OOP_Object *src_bm,
    	    	    	       OOP_Object *dst_bm, OOP_Object *dims_bm)
{
    struct TagItem  	    gctags[] =
    {
    	{ aHidd_GC_DrawMode , vHidd_GC_DrawMode_Copy},
	{ TAG_DONE  	    , 0UL   	    	    }
    };
    struct HIDDGraphicsData *data;
    IPTR    	    	    width, height;
    ULONG   	    	    i, numentries;
    OOP_Object      	    *src_colmap;
    APTR		    psrc_colmap = &src_colmap;
    
    data = OOP_INST_DATA(cl, o);
    
    /* Copy the displayable bitmap into the framebuffer */
    OOP_GetAttr(dims_bm, aHidd_BitMap_Width,	&width);
    OOP_GetAttr(dims_bm, aHidd_BitMap_Height,	&height);
    
    /* We have to copy the colormap into the framebuffer bitmap */
    OOP_GetAttr(src_bm, aHidd_BitMap_ColorMap, (IPTR *)psrc_colmap);
    OOP_GetAttr(src_colmap, aHidd_ColorMap_NumEntries, &numentries);
	
    for (i = 0; i < numentries; i ++)
    {
    	HIDDT_Color col;
	
	HIDD_CM_GetColor(src_colmap, i, &col);
	HIDD_BM_SetColors(dst_bm, &col, i, 1);
    }    

    
    OOP_SetAttrs(data->gc, gctags);
    HIDD_Gfx_CopyBox(o
    	, src_bm
	, 0, 0
    	, dst_bm
	, 0, 0
	, width, height
	, data->gc
    );
}

/****************************************************************************************/

OOP_Object *GFX__Hidd_Gfx__Show(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_Show *msg)
{
    struct HIDDGraphicsData *data;
    OOP_Object      	    *bm;
    IPTR    	    	    displayable;
    
    data = OOP_INST_DATA(cl, o);
    bm = msg->bitMap;
    
    /* We have to do some consistency checking */
    if (bm)
    {
    	OOP_GetAttr(bm, aHidd_BitMap_Displayable, &displayable);
    }
    
    if (bm && !displayable)
    	/* We cannot show a non-displayable bitmap */
	return NULL;
	
    if (NULL == data->framebuffer)
	return NULL;
	
    if (NULL != data->shownbm && (msg->flags & fHidd_Gfx_Show_CopyBack))
    {
    	/* Copy the framebuffer data back into the old shown bitmap */
	copy_bm_and_colmap(cl, o, data->framebuffer, data->shownbm, data->shownbm);
    }

    if (bm)
    {
    	copy_bm_and_colmap(cl, o, bm, data->framebuffer, bm);
    }
    else
    {
    	#warning should clear framebuffer to black
    }
    data->shownbm = bm;
    
    return data->framebuffer;
}

/****************************************************************************************/

BOOL GFX__Hidd_Gfx__SetCursorShape(OOP_Class *cl, OOP_Object *o,
    	    	    	    	   struct pHidd_Gfx_SetCursorShape *msg)
{
    /* We have no clue how to render the cursor */
    return TRUE;
}

/****************************************************************************************/

BOOL GFX__Hidd_Gfx__SetCursorVisible(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorVisible *msg)
{
    return TRUE;
}

/****************************************************************************************/

BOOL GFX__Hidd_Gfx__SetCursorPos(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorPos *msg)
{
    return TRUE;
}

/*****************************************************************************************

    HIDDGfx::CopyBox() 

    NAME
        CopyBox

    SYNOPSIS
        OOP_DoMethod(src, WORD srcX, WORD srcY,
                      OOP_Object *dest, WORD destX, WORD destY,
                      UWORD sizeX, UWORD sizeY);

   FUNCTION
        Copy a rectangular area from the drawing area src to the drawing
        area stored in dest (which may be src). The source area is not
        changed (except when both rectangles overlap). The mode of the GC
        dest determines how the copy takes place.

        In quick mode, the following restrictions are not checked: It's not
        checked whether the source or destination rectangle is completely
        inside the valid area or whether the areas overlap. If they
        overlap, the results are unpredictable. Also drawing modes are
        ignored. If the two bitmaps in the GCs have a different depth,
        copying might be slow.

        When copying bitmaps between two different HIDDs, the following
        pseudo algorithm is executed: First the destination HIDD is queried
        whether it does understand the format of the source HIDD. If it
        does, then the destination HIDD does the copying. If it doesn't,
        then the source is asked whether it understands the destination
        HIDDs' format. If it does, then the source HIDD will do the
        copying. If it doesn't, then the default CopyArea of the graphics
        HIDD base class will be invoked which copies the bitmaps pixel by
        pixel with BitMap::GetPixel() and BitMap::DrawPixel().

    INPUTS
        src           - source bitmap object
        srcX, srcY    - upper, left corner of the area to copy in the source
        dest          - destination bitmap object
        destX, destY  - upper, left corner in the destination to copy the area
        width, height - width and height of the area in hidd units

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        GROUP=HIDD_BltBitMap

    INTERNALS

    TODO

    HISTORY

*****************************************************************************************/

VOID GFX__Hidd_Gfx__CopyBox(OOP_Class *cl, OOP_Object *obj, struct pHidd_Gfx_CopyBox *msg)
{
    WORD    	    	    	    x, y;
    WORD    	    	    	    srcX = msg->srcX, destX = msg->destX;
    WORD    	    	    	    srcY = msg->srcY, destY = msg->destY;
    WORD    	    	    	    startX, endX, deltaX, startY, endY, deltaY;
    ULONG   	    	    	    memFG;
    
    HIDDT_PixelFormat 	    	    *srcpf, *dstpf;
    OOP_Object      	    	    *dest, *src;
    
    OOP_Object      	    	    *gc;
#if USE_FAST_GETPIXEL
    struct pHidd_BitMap_GetPixel    get_p;
#endif

#if USE_FAST_DRAWPIXEL
    struct pHidd_BitMap_DrawPixel   draw_p;
    
    draw_p.mID	= CSD(cl)->drawpixel_mid;
    draw_p.gc	= msg->gc;
#endif

#if USE_FAST_GETPIXEL
    get_p.mID	= CSD(cl)->getpixel_mid;
#endif
    
    dest = msg->dest;
    src  = msg->src;

    /* If source/dest overlap, direction of operation is important */
    
    if (srcX < destX)
    {
    	startX = msg->width - 1;  endX = -1; deltaX = -1;
    }
    else
    {
    	startX = 0; endX = msg->width;  deltaX = 1;
    }
 
    if (srcY < destY)
    {
	startY = msg->height - 1; endY = -1; deltaY = -1;
    }
    else
    {
	startY = 0; endY = msg->height; deltaY = 1;
    }
    
    /* Get the source pixel format */
    srcpf = (HIDDT_PixelFormat *)HBM(src)->prot.pixfmt;
    
    /* bug("COPYBOX: SRC PF: %p, obj=%p, cl=%s, OOP_OCLASS: %s\n", srcpf, obj
	    , cl->ClassNode.ln_Name, OOP_OCLASS(obj)->ClassNode.ln_Name);
    */
    
#if 0
    {
	ULONG sw, sh, dw, dh;
	D(bug("COPYBOX: src=%p, dst=%p, width=%d, height=%d\n"
	    , obj, msg->dest, msg->width, msg->height));

	OOP_GetAttr(obj, aHidd_BitMap_Width, &sw);
	OOP_GetAttr(obj, aHidd_BitMap_Height, &sh);
	OOP_GetAttr(msg->dest, aHidd_BitMap_Width, &dw);
	OOP_GetAttr(msg->dest, aHidd_BitMap_Height, &dh);
	D(bug("src dims: %d, %d  dest dims: %d, %d\n", sw, sh, dw, dh));
    }
#endif

    dstpf = (HIDDT_PixelFormat *)HBM(dest)->prot.pixfmt;
    
    /* Compare graphtypes */
    if (HIDD_PF_COLMODEL(srcpf) == HIDD_PF_COLMODEL(dstpf))
    {
    	/* It is ok to do a direct copy */
    }
    else
    {
    	/* Find out the gfx formats */
	if (  IS_PALETTIZED(srcpf) && IS_TRUECOLOR(dstpf))
	{
	
	}
	else if (IS_TRUECOLOR(srcpf) && IS_PALETTIZED(dstpf))
	{
	
	}
	else if (IS_PALETTE(srcpf) && IS_STATICPALETTE(dstpf)) 
	{
	
	}
	else if (IS_STATICPALETTE(srcpf) && IS_PALETTE(dstpf))
	{
	
	}
    }
    
    gc = msg->gc;
    
    memFG = GC_FG(msg->gc);
    
    /* All else have failed, copy pixel by pixel */


    if (HIDD_PF_COLMODEL(srcpf) == HIDD_PF_COLMODEL(dstpf))
    {
    	if (IS_TRUECOLOR(srcpf))
	{
    	    // bug("COPY FROM TRUECOLOR TO TRUECOLOR\n");
	    for(y = startY; y != endY; y += deltaY)
	    {
		HIDDT_Color col;
		
		/* if (0 == strcmp("CON: Window", FindTask(NULL)->tc_Node.ln_Name))
		    bug("[%d,%d] ", memSrcX, memDestX);
		*/    
		for(x = startX; x != endX; x += deltaX)
		{
		    HIDDT_Pixel pix;
		    
    	    	#if USE_FAST_GETPIXEL
		    get_p.x = srcX + x;
		    get_p.y = srcY + y;
		    pix = GETPIXEL(src, &get_p);
    	    	#else
		    pix = HIDD_BM_GetPixel(obj, srcX + x, srcY + y);
    	    	#endif

    	    	#if COPYBOX_CHECK_FOR_ALIKE_PIXFMT
		    if (srcpf == dstpf)
		    {
			GC_FG(gc) = pix;
		    } 
		    else 
		    {
    	    	#endif
		    HIDD_BM_UnmapPixel(src, pix, &col);
		    GC_FG(gc) = HIDD_BM_MapColor(msg->dest, &col);
    	    	#if COPYBOX_CHECK_FOR_ALIKE_PIXFMT
		    }
    	    	#endif

    	    // #if 0

    	    	#if USE_FAST_DRAWPIXEL
		    draw_p.x = destX + x;
		    draw_p.y = destY + y;
		    DRAWPIXEL(dest, &draw_p);
    	    	#else
		    
		    HIDD_BM_DrawPixel(msg->dest, gc, destX + x, destY + y);
    	    	#endif

    	    // #endif
		}
		/*if (0 == strcmp("CON: Window", FindTask(NULL)->tc_Node.ln_Name))
		    bug("[%d,%d] ", srcY, destY);
		*/
	    }
	    
        } /* if (IS_TRUECOLOR(srcpf)) */
	else
	{
	     /* Two palette bitmaps.
	        For this case we do NOT convert through RGB,
		but copy the pixel indexes directly
	     */
    	    // bug("COPY FROM PALETTE TO PALETTE\n");

    	    #warning This might not work very well with two StaticPalette bitmaps

	    for(y = startY; y != endY; y += deltaY)
	    {
		for(x = startX; x != endX; x += deltaX)
		{
		    GC_FG(gc) = HIDD_BM_GetPixel(src, srcX + x, srcY + y);
		    
		    HIDD_BM_DrawPixel(msg->dest, gc, destX + x, destY + y);
		    
		}
 	    }
	     
	} /* if (IS_TRUECOLOR(srcpf)) else ... */

    } /* if (HIDD_PF_COLMODEL(srcpf) == HIDD_PF_COLMODEL(dstpf)) */
    else
    {
    	/* Two unlike bitmaps */
	if (IS_TRUECOLOR(srcpf))
	{
    	    #warning Implement this
	     D(bug("!! DEFAULT COPYING FROM TRUECOLOR TO PALETTIZED NOT IMPLEMENTED IN BitMap::CopyBox\n"));
	}
	else if (IS_TRUECOLOR(dstpf))
	{
	    /* Get the colortab */
	    HIDDT_Color *ctab = ((HIDDT_ColorLUT *)HBM(src)->colmap)->colors;

    	    // bug("COPY FROM PALETTE TO TRUECOLOR, DRAWMODE %d, CTAB %p\n", GC_DRMD(gc), ctab);
	    
	    for(y = startY; y != endY; y += deltaY)
	    {		
		for(x = startX; x != endX; x += deltaX)
		{
		    register HIDDT_Pixel pix;
		    register HIDDT_Color *col;
		    
		    pix = HIDD_BM_GetPixel(src, srcX + x, srcY + y);
		    col = &ctab[pix];
	
		    GC_FG(gc) = HIDD_BM_MapColor(msg->dest, col);
		    HIDD_BM_DrawPixel(msg->dest, gc, destX + x, destY + y);
		    
		}
	    }	
	}
	
    } /* if (HIDD_PF_COLMODEL(srcpf) == HIDD_PF_COLMODEL(dstpf)) else ... */
    
    GC_FG(gc) = memFG;
}

/****************************************************************************************/

VOID GFX__Hidd_Gfx__ShowImminentReset(OOP_Class *cl, OOP_Object *obj, OOP_Msg msg)
{
}

/****************************************************************************************/

OOP_Object *GFX__Hidd_Gfx__RegisterPixFmt(OOP_Class *cl, OOP_Object *o,
    	    	    	    	    	  struct pHidd_Gfx_RegisterPixFmt *msg)
{
    HIDDT_PixelFormat 	    cmp_pf;
    struct HIDDGraphicsData *data;
    
    struct pfnode   	    *pfnode;
    
    OOP_Object      	    *retpf = NULL;

    memset(&cmp_pf, 0, sizeof(cmp_pf));
    
    data = OOP_INST_DATA(cl, o);
    if (!parse_pixfmt_tags(msg->pixFmtTags, &cmp_pf, 0, CSD(cl)))
    {
    	D(bug("!!! FAILED PARSING TAGS IN Gfx::RegisterPixFmt() !!!\n"));
	return FALSE;
    }

    ObtainSemaphoreShared(&data->pfsema);
    pfnode = find_pixfmt(&data->pflist, &cmp_pf, CSD(cl));
    ReleaseSemaphore(&data->pfsema);
    
    if (NULL == pfnode)
    {
    	retpf = find_stdpixfmt(&cmp_pf, CSD(cl));
    }
    else
    {
    	retpf = pfnode->pixfmt;
	/* Increase pf refcount */
	pfnode->refcount ++;
    }
    

    if (NULL == retpf)
    {
    	struct pfnode *newnode;
    	/* Could not find an alike pf, Create a new pfdb node  */
	
	newnode = AllocMem(sizeof (struct pfnode), MEMF_ANY);
	if (NULL != newnode)
	{
	    
	    /* Since we pass NULL as the taglist below, the PixFmt class will just create a dummy pixfmt */
	    retpf = OOP_NewObject(CSD(cl)->pixfmtclass, NULL, NULL);
	    if (NULL != retpf)
	    {
	    	newnode->pixfmt = retpf;
		
		/* We have one user */
		newnode->refcount = 1;
		
		/* Initialize the pixfmt object the "ugly" way */
		cmp_pf.stdpixfmt = vHidd_StdPixFmt_Unknown;
		memcpy(retpf, &cmp_pf, sizeof (HIDDT_PixelFormat));
		
		#define PF(x) ((HIDDT_PixelFormat *)x)    
		/*
		bug("(%d, %d, %d, %d), (%x, %x, %x, %x), %d, %d, %d, %d\n"
			, PF(&cmp_pf)->red_shift
			, PF(&cmp_pf)->green_shift
			, PF(&cmp_pf)->blue_shift
			, PF(&cmp_pf)->alpha_shift
			, PF(&cmp_pf)->red_mask
			, PF(&cmp_pf)->green_mask
			, PF(&cmp_pf)->blue_mask
			, PF(&cmp_pf)->alpha_mask
			, PF(&cmp_pf)->bytes_per_pixel
			, PF(&cmp_pf)->size
			, PF(&cmp_pf)->depth
			, PF(&cmp_pf)->stdpixfmt
			, HIDD_BP_BITMAPTYPE(&cmp_pf));

		*/
		
    	    	ObtainSemaphore(&data->pfsema);
		AddTail((struct List *)&data->pflist, (struct Node *)newnode);
    	    	ReleaseSemaphore(&data->pfsema);
	    
	    }
	    else
	    {
	    	FreeMem(newnode, sizeof (struct pfnode));
	    
	    }
	}
    }
    
    return retpf;
      
}

/****************************************************************************************/

VOID GFX__Hidd_Gfx__ReleasePixFmt(OOP_Class *cl, OOP_Object *o,
    	    	    	    	  struct pHidd_Gfx_ReleasePixFmt *msg)
{
    struct HIDDGraphicsData *data;
    
    struct objectnode 	    *n, *safe;


/*    bug("release_pixfmt\n");
*/    

    data = OOP_INST_DATA(cl, o);
    
    /* Go through the pixfmt list trying to find the object */
    ObtainSemaphore(&data->pfsema);
      
    ForeachNodeSafe(&data->pflist, n, safe)
    {
    	if (msg->pixFmt == n->object)
	{
	    n->refcount --;
	    if (0 == n->refcount)
	    {	    
	    	/* Remove the node */
		Remove((struct Node *)n);
		
		/* Dispose the pixel format */
		OOP_DisposeObject(n->object);
		
		/* Free the node */
		FreeMem(n, sizeof (struct objectnode));
		
	    }
	    break;
	}
    }
    
    ReleaseSemaphore(&data->pfsema);
}

/*****************************************************************************************

    Gfx::CheckMode()

    This is a method which should be implemented by the subclasses.
    It should look at the supplied pixfmt and sync and see if it is valid.
    This will handle any special cases that are not covered by the 
    pixfmts/syncs supplied in HIDD_Gfx_RegisterModes().
    For example some advanced modes, like 1024x768x24 @ 90 might not be available
    because of lack of bandwidth in the gfx hw.

*****************************************************************************************/

BOOL GFX__Hidd_Gfx__CheckMode(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CheckMode *msg)
{
    /* As a default we allways return TRUE, ie. the mode is OK */
    return TRUE;
}

/****************************************************************************************/

OOP_Object *GFX__Hidd_Gfx__GetPixFmt(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_GetPixFmt *msg) 
{
    OOP_Object *fmt;
    
    if (!IS_REAL_STDPIXFMT(msg->stdPixFmt))
    {
    	D(bug("!!! Illegal pixel format passed to Gfx::GetPixFmt(): %d\n", msg->stdPixFmt));
	return NULL;
    } 
    else 
    {
    	fmt = CSD(cl)->std_pixfmts[REAL_STDPIXFMT_IDX(msg->stdPixFmt)];
    }

    return fmt;
}

/****************************************************************************************/

#undef csd

/****************************************************************************************/

static int GFX_ClassInit(LIBBASETYPEPTR LIBBASE)
{
    struct class_static_data *csd = &LIBBASE->hdg_csd;
    
    __IHidd_PixFmt  	= OOP_ObtainAttrBase(IID_Hidd_PixFmt);
    __IHidd_BitMap  	= OOP_ObtainAttrBase(IID_Hidd_BitMap);
    __IHidd_Gfx     	= OOP_ObtainAttrBase(IID_Hidd_Gfx);
    __IHidd_Sync    	= OOP_ObtainAttrBase(IID_Hidd_Sync);
    __IHidd_GC      	= OOP_ObtainAttrBase(IID_Hidd_GC);
    __IHidd_ColorMap 	= OOP_ObtainAttrBase(IID_Hidd_ColorMap);
    __IHidd_PlanarBM	= OOP_ObtainAttrBase(IID_Hidd_PlanarBM);
    
    if (!__IHidd_PixFmt     ||
     	!__IHidd_BitMap     ||
	!__IHidd_Gfx 	    ||
	!__IHidd_Sync 	    ||
	!__IHidd_GC 	    ||
	!__IHidd_ColorMap   ||
	!__IHidd_PlanarBM
       )
    {
	goto failexit;
    }

    D(bug("Creating std pixelfmts\n"));
    if (!create_std_pixfmts(csd))
    	goto failexit;
    D(bug("Pixfmts created\n"));

    /* Get two methodis required for direct method execution */
#if USE_FAST_PUTPIXEL
    csd->putpixel_mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutPixel);
#endif
#if USE_FAST_GETPIXEL
    csd->getpixel_mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_GetPixel);
#endif
#if USE_FAST_DRAWPIXEL
    csd->drawpixel_mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_DrawPixel);
#endif

    ReturnInt("init_gfxhiddclass", ULONG, TRUE);
    
failexit:
    ReturnPtr("init_gfxhiddclass", ULONG, FALSE);
}

/****************************************************************************************/

static int GFX_ClassFree(LIBBASETYPEPTR LIBBASE)
{
    struct class_static_data *csd = &LIBBASE->hdg_csd;
    
    EnterFunc(bug("free_gfxhiddclass(csd=%p)\n", csd));
    
    if(NULL != csd)
    {
	delete_std_pixfmts(csd);
        
    	OOP_ReleaseAttrBase(IID_Hidd_PixFmt);
    	OOP_ReleaseAttrBase(IID_Hidd_BitMap);
    	OOP_ReleaseAttrBase(IID_Hidd_Gfx);
    	OOP_ReleaseAttrBase(IID_Hidd_Sync);
    	OOP_ReleaseAttrBase(IID_Hidd_GC);
    	OOP_ReleaseAttrBase(IID_Hidd_ColorMap);
    	OOP_ReleaseAttrBase(IID_Hidd_PlanarBM);

    }
    
    ReturnInt("free_gfxhiddclass", BOOL, TRUE);
}

/****************************************************************************************/

ADD2INITLIB(GFX_ClassInit, 0)
ADD2EXPUNGELIB(GFX_ClassFree, 0)

/****************************************************************************************/

/* Since the shift/mask values of a pixel format are designed for pixel
   access, not byte access, they are endianess dependant */
   
#if AROS_BIG_ENDIAN
#include "stdpixfmts_be.h"
#else
#include "stdpixfmts_le.h"
#endif

/****************************************************************************************/

static BOOL create_std_pixfmts(struct class_static_data *csd)
{
    ULONG i;
    
    memset(csd->std_pixfmts, 0, sizeof (OOP_Object *) * num_Hidd_StdPixFmt);
    
    for (i = 0; i < num_Hidd_StdPixFmt; i ++)
    {
    	csd->std_pixfmts[i] = create_and_init_object(csd->pixfmtclass
		    , (UBYTE *)&stdpfs[i],  sizeof (stdpfs[i]), csd);
		    
	if (NULL == csd->std_pixfmts[i])
	{
	    D(bug("FAILED TO CREATE PIXEL FORMAT %d\n", i));
	    delete_std_pixfmts(csd);
	    ReturnBool("create_stdpixfmts", FALSE);
	}
    }
    ReturnBool("create_stdpixfmts", TRUE);

}

/****************************************************************************************/

static VOID delete_std_pixfmts(struct class_static_data *csd)
{
    ULONG i;
    
    for (i = 0; i < num_Hidd_StdPixFmt; i ++)
    {
    
        if (NULL != csd->std_pixfmts[i])
	{
	    OOP_DisposeObject(csd->std_pixfmts[i]);
	    csd->std_pixfmts[i] = NULL;
	}
    }
}

/****************************************************************************************/

static VOID free_objectlist(struct List *list, BOOL OOP_DisposeObjects,
    	    	    	    struct class_static_data *csd)
{
    struct objectnode *n, *safe;
    
    ForeachNodeSafe(list, n, safe)
    {
    	Remove(( struct Node *)n);
	
	if (NULL != n->object && OOP_DisposeObjects)
	{
	    OOP_DisposeObject(n->object);
	}
	
	FreeMem(n, sizeof (struct objectnode));
    }

}

/****************************************************************************************/

static inline BOOL cmp_pfs(HIDDT_PixelFormat *tmppf, HIDDT_PixelFormat *dbpf)
{
    if (    dbpf->depth == tmppf->depth 
	 && HIDD_PF_COLMODEL(dbpf) == HIDD_PF_COLMODEL(tmppf))
    {
    	/* The pixfmts are very alike, check all attrs */
	     
    	HIDDT_PixelFormat *old;
	
	old = (HIDDT_PixelFormat *)tmppf->stdpixfmt;
	
	tmppf->stdpixfmt = ((HIDDT_PixelFormat *)dbpf)->stdpixfmt;
	     
	if (0 == memcmp(tmppf, dbpf, sizeof (HIDDT_PixelFormat)))
	{
	    return TRUE;
	}
	
	tmppf->stdpixfmt = old;
    }
    
    return FALSE;
}

/****************************************************************************************/

/*
     Matches the supplied pixelformat against all standard pixelformats
     to see if there allready exsts a pixelformat object for this pixelformat
*/

/****************************************************************************************/

static OOP_Object *find_stdpixfmt(HIDDT_PixelFormat *tofind,
    	    	    	    	  struct class_static_data *csd)
{
    OOP_Object *retpf = NULL;
    OOP_Object *stdpf;
    ULONG   	i;
    
    for (i = 0; i < num_Hidd_StdPixFmt; i ++)
    {
    	stdpf = csd->std_pixfmts[i];
	
	if (cmp_pfs(tofind, (HIDDT_PixelFormat *)stdpf))
	{
	    retpf =  stdpf;
	    break;
	}
	
    }
   
    return retpf;
}

/****************************************************************************************/

/*
    Parses the tags supplied in 'tags' and puts the result into 'sync'.
    It also checks to see if all needed attrs are supplied.
    It uses 'attrcheck' for this, so you may find attrs outside
    of this function, and mark them as found before calling this function
*/

#define SYNC_AF(code) (1L << aoHidd_Sync_ ## code)
#define SYAO(x) (aoHidd_Sync_ ## x)

#define X11_SYNC_AF	( \
	  SYNC_AF(HSyncStart) | SYNC_AF(HSyncEnd) | SYNC_AF(HTotal) \
	| SYNC_AF(VSyncStart) | SYNC_AF(VSyncEnd) | SYNC_AF(VTotal) ) 

#define LINUXFB_SYNC_AF ( \
	  SYNC_AF(LeftMargin)  | SYNC_AF(RightMargin) | SYNC_AF(HSyncLength) \
	| SYNC_AF(UpperMargin) | SYNC_AF(LowerMargin) | SYNC_AF(VSyncLength) )

#define SYNC_DISP_AF ( \
	SYNC_AF(HDisp) | SYNC_AF(VDisp) )

/****************************************************************************************/

BOOL parse_sync_tags(struct TagItem *tags, struct sync_data *data, ULONG ATTRCHECK(sync),
    	    	     struct class_static_data *csd)
{
    IPTR attrs[num_Hidd_Sync_Attrs];
    BOOL ok = FALSE;
    
    if (0 != OOP_ParseAttrs(tags, attrs, num_Hidd_Sync_Attrs, &ATTRCHECK(sync), HiddSyncAttrBase))
    {
	D(bug("!!! parse_sync_tags: ERROR PARSING ATTRS !!!\n"));
	return FALSE;
    }

    /* Check that we have all attrs */
    if (GOT_SYNC_ATTR(PixelTime))
    {
	data->pixtime = attrs[SYAO(PixelTime)];
    }
    else if (GOT_SYNC_ATTR(PixelClock))
    {
    #if !AROS_BOCHS_HACK
    #if AROS_NOFPU
    	#warning Write code for non-FPU!
	data->pixtime = 0x12345678;
    	#else
	/* Something in there makes Bochs freeze */
	DOUBLE pixclock, pixtime;
		    
	/* Compute the pixtime */
	pixclock =(DOUBLE)attrs[SYAO(PixelClock)];
	pixtime = 1 / pixclock;
	pixtime *= 1000000000000;
	data->pixtime = (ULONG)pixtime;
    #endif
    #else
	data->pixtime = 0x12345678;
    #endif	
    }
    else
    {
	D(bug("!!! MISSING PIXELTIME/CLOCK ATTR !!!\n"));
	return FALSE;
    }
    
    if (GOT_SYNC_ATTR(Description))
    {
    	strlcpy(data->description,
	    	(STRPTR)attrs[SYAO(Description)],
		sizeof(data->description));
    }
    
    /* Check that we have HDisp and VDisp */
    if (SYNC_DISP_AF != (SYNC_DISP_AF & ATTRCHECK(sync)))
    {
	D(bug("!!! MISSING HDISP OR VDISP ATTR !!!\n"));
    }
    else
    {
	data->hdisp = attrs[SYAO(HDisp)];
	data->vdisp = attrs[SYAO(VDisp)];

	/* Test that the user has not supplied both X11 style and fbdev style attrs */
	if ( (LINUXFB_SYNC_AF & ATTRCHECK(sync)) != 0 && (X11_SYNC_AF & ATTRCHECK(sync)) != 0 )
	{
	    D(bug("!!! BOTH LINUXFB-STYLE AND X11-STYLE ATTRS WERE SUPPLIED !!!\n"));
	    D(bug("!!! YOU MAY ONLY SUPPLY ONE OF THEM !!!\n"));
	}
	else
	{
	    
	    /* Test that we have all attrs of either the X11 style or the Linux FB style */
	    if ((LINUXFB_SYNC_AF & ATTRCHECK(sync)) == LINUXFB_SYNC_AF)
	    {
		/* Set the data struct */
		data->left_margin	= attrs[SYAO(LeftMargin)];
		data->right_margin	= attrs[SYAO(RightMargin)];
		data->hsync_length	= attrs[SYAO(HSyncLength)];
		
		data->upper_margin	= attrs[SYAO(UpperMargin)];
		data->lower_margin	= attrs[SYAO(LowerMargin)];
		data->vsync_length	= attrs[SYAO(VSyncLength)];
		ok = TRUE;
		
	    }
	    else if ((X11_SYNC_AF & ATTRCHECK(sync)) == X11_SYNC_AF)
	    {
		ULONG hsync_start, hsync_end, htotal;
		ULONG vsync_start, vsync_end, vtotal;
		
		hsync_start	= attrs[SYAO(HSyncStart)];
		hsync_end	= attrs[SYAO(HSyncEnd)];
		htotal		= attrs[SYAO(HTotal)];
		
		vsync_start	= attrs[SYAO(VSyncStart)];
		vsync_end	= attrs[SYAO(VSyncEnd)];
		vtotal		= attrs[SYAO(VTotal)];

		data->left_margin  = htotal      - hsync_end;
		data->right_margin = hsync_start - data->hdisp;
		data->hsync_length = hsync_end   - hsync_start;

		data->upper_margin = vtotal      - vsync_end;
		data->lower_margin = vsync_start - data->vdisp;
		data->vsync_length = vsync_end   - vsync_start;
		ok = TRUE;
	    }
	    else
	    {
		D(bug("!!! UNSUFFICIENT ATTRS PASSED TO parse_sync_tags: %x !!!\n", ATTRCHECK(sync)));
	    }
	}
    }
    
    return ok;
}

/****************************************************************************************/

/*
    Parses the tags supplied in 'tags' and puts the result into 'pf'.
    It also checks to see if all needed attrs are supplied.
    It uses 'attrcheck' for this, so you may find attrs outside
    of this function, and mark them as found before calling this function
*/

#define PFAF(x) (1L << aoHidd_PixFmt_ ## x)
#define PF_COMMON_AF (   PFAF(Depth) | PFAF(BitsPerPixel) | PFAF(BytesPerPixel)	\
		       | PFAF(ColorModel) | PFAF(BitMapType) )
		       
#define PF_TRUECOLOR_AF ( PFAF(RedMask)  | PFAF(GreenMask)  | PFAF(BlueMask)  | PFAF(AlphaMask) | \
			  PFAF(RedShift) | PFAF(GreenShift) | PFAF(BlueShift) | PFAF(AlphaShift))
			  
#define PF_PALETTE_AF ( PFAF(CLUTMask) | PFAF(CLUTShift) | PFAF(RedMask) | PFAF(GreenMask) | \
			PFAF(BlueMask) )
		       
#define PFAO(x) (aoHidd_PixFmt_ ## x)
  
/****************************************************************************************/

BOOL parse_pixfmt_tags(struct TagItem *tags, HIDDT_PixelFormat *pf,
    	    	       ULONG ATTRCHECK(pixfmt), struct class_static_data *csd)
{
    IPTR attrs[num_Hidd_PixFmt_Attrs];
     
    if (0 != OOP_ParseAttrs(tags, attrs, num_Hidd_PixFmt_Attrs,
    	    	    	    &ATTRCHECK(pixfmt), HiddPixFmtAttrBase))
    {
	D(bug("!!! parse_pixfmt_tags: ERROR PARSING TAGS THROUGH OOP_ParseAttrs !!!\n"));
	return FALSE;
    }

    if (PF_COMMON_AF != (PF_COMMON_AF & ATTRCHECK(pixfmt)))
    {
	D(bug("!!! parse_pixfmt_tags: Missing PixFmt attributes passed to parse_pixfmt_tags(): %x !!!\n", ATTRCHECK(pixfmt)));
	return FALSE;
    }
    
    /* Set the common attributes */
    pf->depth		= attrs[PFAO(Depth)];
    pf->size		= attrs[PFAO(BitsPerPixel)];
    pf->bytes_per_pixel	= attrs[PFAO(BytesPerPixel)];
    
    SET_PF_COLMODEL(  pf, attrs[PFAO(ColorModel)]);
    SET_PF_BITMAPTYPE(pf, attrs[PFAO(BitMapType)]);
    
    if (ATTRCHECK(pixfmt) & PFAF(SwapPixelBytes))
    {
    	SET_PF_SWAPPIXELBYTES_FLAG(pf, attrs[PFAO(SwapPixelBytes)]);
    }
    
    /* Set the colormodel specific stuff */
    switch (HIDD_PF_COLMODEL(pf))
    {
    	case vHidd_ColorModel_TrueColor:
	    /* Check that we got all the truecolor describing stuff */
	    if (PF_TRUECOLOR_AF != (PF_TRUECOLOR_AF & ATTRCHECK(pixfmt)))
	    {
		 D(bug("!!! Unsufficient true color format describing attrs to pixfmt in parse_pixfmt_tags() !!!\n"));
		 return FALSE;
	    }
	    
	    /* Set the truecolor stuff */
	    pf->red_mask	= attrs[PFAO(RedMask)];
	    pf->green_mask	= attrs[PFAO(GreenMask)];
	    pf->blue_mask	= attrs[PFAO(BlueMask)];
	    pf->alpha_mask	= attrs[PFAO(AlphaMask)];
	    
	    pf->red_shift	= attrs[PFAO(RedShift)];
	    pf->green_shift	= attrs[PFAO(GreenShift)];
	    pf->blue_shift	= attrs[PFAO(BlueShift)];
	    pf->alpha_shift	= attrs[PFAO(AlphaShift)];
	    break;
	
	case vHidd_ColorModel_Palette:
	case vHidd_ColorModel_StaticPalette:
	    if ( PF_PALETTE_AF != (PF_PALETTE_AF & ATTRCHECK(pixfmt)))
	    {
		 D(bug("!!! Unsufficient palette format describing attrs to pixfmt in parse_pixfmt_tags() !!!\n"));
		 return FALSE;
	    }
	    
	    /* set palette stuff */
	    pf->clut_mask	= attrs[PFAO(CLUTMask)];
	    pf->clut_shift	= attrs[PFAO(CLUTShift)];

	    pf->red_mask	= attrs[PFAO(RedMask)];
	    pf->green_mask	= attrs[PFAO(GreenMask)];
	    pf->blue_mask	= attrs[PFAO(BlueMask)];

	    break;
	    
    } /* shift (colormodel) */
    
    return TRUE;
}

/****************************************************************************************/

/* 
    Create an empty object and initialize it the "ugly" way. This only works with
    CLID_Hidd_PixFmt and CLID_Hidd_Sync classes
*/

/****************************************************************************************/

static OOP_Object *create_and_init_object(OOP_Class *cl, UBYTE *data, ULONG datasize,
    	    	    	    	    	  struct class_static_data *csd)
{
    OOP_Object *o;
			
    o = OOP_NewObject(cl, NULL, NULL);
    if (NULL == o)
    {
	D(bug("!!! UNABLE TO CREATE OBJECT IN create_and_init_object() !!!\n"));
	return NULL;
    }
	    
    memcpy(o, data, datasize);
    
    return o;
}
	
/****************************************************************************************/

static struct pfnode *find_pixfmt(struct MinList *pflist, HIDDT_PixelFormat *tofind,
    	    	    	    	  struct class_static_data *csd)
{
    OOP_Object      	*retpf = NULL;
    HIDDT_PixelFormat 	*db_pf;
    struct pfnode   	*n;
    
    /* Go through the pixel format list to see if a similar pf allready exists */
    ForeachNode(pflist, n)
    {
    	db_pf = (HIDDT_PixelFormat *)n->pixfmt;
	if (cmp_pfs(tofind, db_pf))
	{
	    retpf = (OOP_Object *)db_pf;
	    break;
	}
    }
    
    if (NULL != retpf)
    	return n;
	
    return NULL;
}

/****************************************************************************************/

/* Stubs for private methods */

#ifndef AROS_CREATE_ROM
#  define STATIC_MID static OOP_MethodID mid
#else
#  define STATIC_MID OOP_MethodID mid = 0
#endif

/****************************************************************************************/

OOP_Object *HIDD_Gfx_RegisterPixFmt(OOP_Object *o, struct TagItem *pixFmtTags)
{
   STATIC_MID;  
   struct pHidd_Gfx_RegisterPixFmt p, *msg = &p;
   
   if (!mid) mid = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_RegisterPixFmt);
   
   p.mID = mid;
   
   p.pixFmtTags = pixFmtTags;
   
   return (OOP_Object *)OOP_DoMethod(o, (OOP_Msg)msg);
   
}

/****************************************************************************************/

VOID HIDD_Gfx_ReleasePixFmt(OOP_Object *o, OOP_Object *pixFmt)
{
   STATIC_MID;  
   struct pHidd_Gfx_ReleasePixFmt p, *msg = &p;
   
   if (!mid) mid = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_ReleasePixFmt);
   
   p.mID = mid;
   
   p.pixFmt = pixFmt;
   
   OOP_DoMethod(o, (OOP_Msg)msg);
   
}

/****************************************************************************************/


