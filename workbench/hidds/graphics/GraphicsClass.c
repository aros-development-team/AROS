/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: Graphics hidd class implementation.
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE 1
#include <exec/lists.h>

#include "graphics_intern.h"

#include <string.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <aros/machine.h>

#include <utility/tagitem.h>
#include <hidd/graphics.h>

#undef  SDEBUG
#undef  DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

static BOOL create_std_pixfmts(struct class_static_data *csd);
static VOID delete_std_pixfmts(struct class_static_data *csd);
static VOID free_objectlist(struct List *list, BOOL disposeobjects, struct class_static_data *csd);
static BOOL register_modes(Class *cl, Object *o, struct TagItem *modetags);

static BOOL alloc_mode_db(struct mode_db *mdb, ULONG numsyncs, ULONG numpfs, Class *cl);
static VOID free_mode_db(struct mode_db *mdb, Class *cl);
static Object *create_and_init_object(Class *cl, UBYTE *data, ULONG datasize, struct class_static_data *csd);
static VOID draw_cursor(struct HIDDGraphicsData *data, BOOL draw, struct class_static_data *csd);

static struct pfnode *find_pixfmt(struct MinList *pflist
	, HIDDT_PixelFormat *tofind
	, struct class_static_data *csd);

static Object *find_stdpixfmt(HIDDT_PixelFormat *tofind
	, struct class_static_data *csd);

/*static AttrBase HiddGCAttrBase;*/

static AttrBase HiddPixFmtAttrBase	= 0;
static AttrBase HiddBitMapAttrBase	= 0;
static AttrBase HiddGfxAttrBase		= 0;
static AttrBase HiddSyncAttrBase	= 0;
static AttrBase HiddGCAttrBase		= 0;
static struct ABDescr attrbases[] = {
    { IID_Hidd_PixFmt, 	&HiddPixFmtAttrBase	},
    { IID_Hidd_BitMap,	&HiddBitMapAttrBase	},
    { IID_Hidd_Gfx,	&HiddGfxAttrBase	},
    { IID_Hidd_Sync,	&HiddSyncAttrBase	},
    { IID_Hidd_GC,	&HiddGCAttrBase		},
    { NULL, NULL }
};
    

BOOL parse_pixfmt_tags(struct TagItem *tags, HIDDT_PixelFormat *pf, ULONG attrcheck, struct class_static_data *csd);
BOOL parse_sync_tags(struct TagItem *tags, struct sync_data *data, ULONG attrcheck, struct class_static_data *csd);



#define COMPUTE_HIDD_MODEID(sync, pf)	\
    ( ((sync) << 16) | (pf) )
    
#define MODEID_TO_SYNCIDX(id) ( (id) >> 16 )
#define MODEID_TO_PFIDX(id) ( (id) & 0x0000FFFF )


/*** HIDDGfx::New() *********************************************************/

static Object *root_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    struct HIDDGraphicsData *data;
    BOOL ok = FALSE;
    struct TagItem *modetags;
    struct TagItem gctags[] = { {TAG_DONE, 0UL} };
    
    o = (Object *)DoSuperMethod(cl, o, (Msg)msg);
    if (NULL == o)
    	return NULL;
    
    data = INST_DATA(cl, o);
    
    NEWLIST(&data->pflist);
    
    InitSemaphore(&data->mdb.sema);
    InitSemaphore(&data->pfsema);
    
    /* Get the mode tags */
    modetags = (struct TagItem *)GetTagData(aHidd_Gfx_ModeTags, NULL, msg->attrList);
    if (NULL != modetags) {
	/* Parse it and register the gfxmodes */
	if (register_modes(cl, o, modetags)) {
	    ok = TRUE;
	}
    }
    
    /* Create a gc that we can use for some rendering */
    if (ok) {
	data->gc = NewObject(CSD(cl)->gcclass, NULL, gctags);
	if (NULL == data->gc)
	    ok = FALSE;
    }
    
    if (!ok) {
	MethodID dispose_mid;
	dispose_mid = GetMethodID(IID_Root, moRoot_Dispose);
	CoerceMethod(cl, o, (Msg)&dispose_mid);
	o = NULL;
    }
    
    return o;
}


/*** HIDDGfx::Dispose() *********************************************************/

static VOID root_dispose(Class *cl, Object *o, Msg msg)
{
    struct HIDDGraphicsData *data;
    data = INST_DATA(cl, o);
    
    /* free the mode db stuff */
    free_mode_db(&data->mdb, cl);
    
    
    ObtainSemaphore(&data->pfsema);
    free_objectlist((struct List *)&data->pflist, TRUE, CSD(cl));
    ReleaseSemaphore(&data->pfsema);
    
    if (NULL != data->gc)
	DisposeObject(data->gc);

    DoSuperMethod(cl, o, msg);
}

/*** HIDDGfx::Get() *********************************************************/

static VOID root_get(Class *cl, Object *o, struct pRoot_Get *msg)
{
    struct HIDDGraphicsData *data;
    BOOL found = FALSE;
    ULONG idx;
    
    data = INST_DATA(cl, o);
    
    if (IS_GFX_ATTR(msg->attrID, idx)) {
	switch (idx) {
	    case aoHidd_Gfx_NumSyncs: {
		found = TRUE;
		*msg->storage = data->mdb.num_syncs;
		break;
	    }
	    
	    default:	/* Keep compiler happy */
		break;
	}
    }
    
    if (!found)
        DoSuperMethod(cl, o, (Msg)msg);
	
    return;    
}



/*** HIDDGfx::NewGC() *********************************************************/

static Object *hiddgfx_newgc(Class *cl, Object *o, struct pHidd_Gfx_NewGC *msg)
{
    Object *gc = NULL;

    EnterFunc(bug("HIDDGfx::NewGC()\n"));

    gc = NewObject(NULL, CLID_Hidd_GC, msg->attrList);


    ReturnPtr("HIDDGfx::NewGC", Object *, gc);
}


/*** HIDDGfx::DisposeGC() ****************************************************/

static VOID hiddgfx_disposegc(Class *cl, Object *o, struct pHidd_Gfx_DisposeGC *msg)
{
    EnterFunc(bug("HIDDGfx::DisposeGC()\n"));

    if (NULL != msg->gc) DisposeObject(msg->gc);

    ReturnVoid("HIDDGfx::DisposeGC");
}


/*** HIDDGfx::NewBitMap() ****************************************************/

#define BMAO(x) aoHidd_BitMap_ ## x
#define BMAF(x) (1L << aoHidd_BitMap_ ## x)

#define BM_DIMS_AF  (BMAF(Width) | BMAF(Height))

#define SET_TAG(tags, idx, tag, val)	\
    tags[idx].ti_Tag = tag ; tags[idx].ti_Data = (IPTR)val;

#define SET_BM_TAG(tags, idx, tag, val)	\
    SET_TAG(tags, idx, aHidd_BitMap_ ## tag, val)

	
static Object * hiddgfx_newbitmap(Class *cl, Object *o, struct pHidd_Gfx_NewBitMap *msg)
{
    struct TagItem bmtags[7];
    
    IPTR attrs[num_Total_BitMap_Attrs];
    STRPTR classid = NULL;
    Class *classptr = NULL;
    BOOL displayable = FALSE; /* Default attr value */
    Object *pf = NULL, *sync;
    HIDDT_ModeID modeid;
    
    DECLARE_ATTRCHECK(bitmap);
    
    BOOL gotclass = FALSE;
    
    
    if (0 != ParseAttrs(msg->attrList, attrs, num_Total_BitMap_Attrs
    	, &ATTRCHECK(bitmap), HiddBitMapAttrBase)) {
	kprintf("!!! FAILED TO PARSE ATTRS IN Gfx::NewBitMap !!!\n");
	return NULL;
    }
    
    if (GOT_BM_ATTR(PixFmt)) {
	kprintf("!!! Gfx::NewBitMap: USER IS NOT ALLOWED TO PASS aHidd_BitMap_PixFmt !!!\n");
	return NULL;
    }
    
    /* Get class supplied by superclass */
    if (GOT_BM_ATTR(ClassPtr)) {
    	classptr	= (Class *)attrs[BMAO(ClassPtr)];
	gotclass = TRUE;
    } else {
	if (GOT_BM_ATTR(ClassID)) {
    	    classid	= (STRPTR)attrs[BMAO(ClassID)];
	    gotclass = TRUE;
	}
    }
		
    if (GOT_BM_ATTR(Displayable))
	displayable = (BOOL)attrs[BMAO(Displayable)];

    if (GOT_BM_ATTR(ModeID)) {
	modeid = attrs[BMAO(ModeID)];
	/* Check that it is a valid mode */
	if (!HIDD_Gfx_GetMode(o, modeid, &sync, &pf)) {
	    kprintf("!!! Gfx::NewBitMap: USER PASSED INVALID MODEID !!!\n");
	}
    }

    /* First argument is gfxhidd */    
    SET_BM_TAG(bmtags, 0, GfxHidd, o);
    SET_BM_TAG(bmtags, 1, Displayable, displayable);
	
    if (displayable) {
	/* The user has to supply a modeid */
	if (!GOT_BM_ATTR(ModeID)) {
	    kprintf("!!! Gfx::NewBitMap: USER HAS NOT PASSED MODEID FOR DISPLAYABLE BITMAP !!!\n");
	    return NULL;
	}
	
	if (!gotclass) {
	    kprintf("!!! Gfx::NewBitMap: SUBCLASS DID NOT PASS CLASS FOR DISPLAYABLE BITMAP !!!\n");
	    return NULL;
	}
	
	SET_BM_TAG(bmtags, 2, ModeID, modeid);
	SET_TAG(bmtags, 3, TAG_MORE, msg->attrList);
	
    } else { /* if (displayable) */
	ULONG width, height;
    
	/* To get a pixfmt for an offscreen bitmap we either need 
	    (ModeID || ( (Width && Height) && StdPixFmt) || ( (Width && Height) && Friend))
	*/
	    
	if (GOT_BM_ATTR(ModeID)) {   
	    /* We have allredy gotten pixelformat and sync for the modeid case */
	    GetAttr(sync, aHidd_Sync_HDisp, &width);
	    GetAttr(sync, aHidd_Sync_VDisp, &height);
	} else {
	    /* Next to look for is StdPixFmt */
	    
	    /* Check that we have width && height */
	    if (BM_DIMS_AF != (BM_DIMS_AF & ATTRCHECK(bitmap))) {
		kprintf("!!! Gfx::NewBitMap() MISSING WIDTH/HEIGHT TAGS !!!\n");
		return NULL;
	    }
	    width  = attrs[BMAO(Width)];
	    height = attrs[BMAO(Height)];
	    
	    
	    if (GOT_BM_ATTR(StdPixFmt)) {

		pf = HIDD_Gfx_GetPixFmt(o, (HIDDT_StdPixFmt)attrs[BMAO(StdPixFmt)]);
		if (NULL == pf) {
		    kprintf("!!! Gfx::NewBitMap(): USER PASSED BOGUS StdPixFmt !!!\n");
		    return NULL;
		}
	    } else {
		/* Last alternative is that the user passed a friend bitmap */
		if (GOT_BM_ATTR(Friend)) {
		    GetAttr((Object *)attrs[BMAO(Friend)], aHidd_BitMap_PixFmt, (IPTR *)&pf);
		} else {
		    kprintf("!!! Gfx::NewBitMap: UNSIFFICIENT ATTRS TO CREATE OFFSCREEN BITMAP !!!\n");
		    return NULL;
		}
	    }
	}
	
	/* Did the subclass provide an offbitmap class for us ? */
	if (!gotclass) {
	    /* Have to find a suitable class ourselves */
	    HIDDT_BitMapType bmtype;
		
	    GetAttr(pf, aHidd_PixFmt_BitMapType, &bmtype);
	    switch (bmtype) {
	        case vHidd_BitMapType_Chunky: classptr = CSD(cl)->chunkybmclass; break;
	        case vHidd_BitMapType_Planar: classptr = CSD(cl)->planarbmclass; break;
	        default:
	    	    kprintf("!!! Gfx::NewBitMap: UNKNOWN BITMAPTYPE %d !!!\n", bmtype);
		    return NULL;
	    }
	} /* if (!gotclass) */
	
	/* Set the tags we want to pass to the selected bitmap class */
	SET_BM_TAG(bmtags, 2, Width,  width);
	SET_BM_TAG(bmtags, 3, Height, height);
	SET_BM_TAG(bmtags, 4, PixFmt, pf);
	if (GOT_BM_ATTR(Friend)) {
	    SET_BM_TAG(bmtags, 5, Friend, attrs[BMAO(Friend)]);
	} else {
	    SET_TAG(bmtags, 5, TAG_IGNORE, 0UL);
	}
	SET_TAG(bmtags, 6, TAG_MORE, msg->attrList);
    } /* if (!displayable) */
    

    return NewObject(classptr, classid, bmtags);
}


/*** HIDDGfx::DisposeBitMap() ************************************************/

static VOID hiddgfx_disposebitmap(Class *cl, Object *o, struct pHidd_Gfx_DisposeBitMap *msg)
{
    if (NULL != msg->bitMap)
	DisposeObject(msg->bitMap);
	

}




/*** register_modes() ********************************************/
#define SD(x) ((struct sync_data *)x)
#define PF(x) ((HIDDT_PixelFormat *)x)

#define XCOORD_TO_BYTEIDX(x) ( (x) >> 3)
#define COORD_TO_BYTEIDX(x, y, bpr) ( ( (y) * bpr ) + XCOORD_TO_BYTEIDX(x) )
#define XCOORD_TO_MASK(x) (1L << (7 - ((x) & 0x07) ))
#define WIDTH_TO_BYTES(width) ( (( (width) - 1) >> 3) + 1)

/* modebm functions pfidx is x and syncidx is y coord in the bitmap */
static inline BOOL alloc_mode_bm(struct mode_bm *bm, ULONG numsyncs, ULONG numpfs, Class *cl)
{
    bm->bpr = WIDTH_TO_BYTES(numpfs);
    bm->bm = AllocVec(bm->bpr * numsyncs, MEMF_CLEAR);
    if (NULL == bm->bm)
	return FALSE;
	
    /* We initialize the mode bitmap to all modes valid */
    memset(bm->bm, 0xFF, bm->bpr * numsyncs);
    
    return TRUE;
}

static inline VOID free_mode_bm(struct mode_bm *bm, Class *cl)
{
    FreeVec(bm->bm);
    bm->bm  = NULL;
    bm->bpr = 0;
}

static inline BOOL is_valid_mode(struct mode_bm *bm, ULONG syncidx, ULONG pfidx)
{
    if (0 != (XCOORD_TO_MASK(pfidx) & bm->bm[COORD_TO_BYTEIDX(pfidx, syncidx, bm->bpr)]))
	return TRUE;
    return FALSE;
}


static inline VOID set_valid_mode(struct mode_bm *bm, ULONG syncidx, ULONG pfidx, BOOL valid)
{
    if (valid)
	bm->bm[COORD_TO_BYTEIDX(pfidx, syncidx, bm->bpr)] |= XCOORD_TO_MASK(pfidx);
    else
	bm->bm[COORD_TO_BYTEIDX(pfidx, syncidx, bm->bpr)] &= ~XCOORD_TO_MASK(pfidx);

    return;
}

static BOOL alloc_mode_db(struct mode_db *mdb, ULONG numsyncs, ULONG numpfs, Class *cl)
{
    BOOL ok = FALSE;
    
    if (0 == numsyncs || 0 == numpfs)
	return FALSE;
	
    ObtainSemaphore(&mdb->sema);
    /* free_mode_bm() needs this */
    mdb->num_pixfmts	= numpfs;
    mdb->num_syncs	= numsyncs;

    mdb->syncs	 = AllocMem(sizeof (Object *) * numsyncs, MEMF_CLEAR);
    if (NULL != mdb->syncs) {
	mdb->pixfmts = AllocMem(sizeof (Object *) * numpfs,   MEMF_CLEAR);
	if (NULL != mdb->pixfmts) {
	    if (alloc_mode_bm(&mdb->orig_mode_bm, numsyncs, numpfs, cl)) {
		if (alloc_mode_bm(&mdb->checked_mode_bm, numsyncs, numpfs, cl)) {
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

static VOID free_mode_db(struct mode_db *mdb, Class *cl)
{
    ULONG i;
    ObtainSemaphore(&mdb->sema);
    if (NULL != mdb->pixfmts) {
    
/****** !!! NB !!! The Pixel formats are registerd in the
  GfxMode Database and is freed in the
  free_object_list functions

    	for (i = 0; i < mdb->num_pixfmts; i ++) {
	    if (NULL != mdb->pixfmts[i]) {
	    	DisposeObject(mdb->pixfmts[i]);
		mdb->pixfmts[i] = NULL;
	    }
	}
*/

	FreeMem(mdb->pixfmts, sizeof (Object *) * mdb->num_pixfmts);
	mdb->pixfmts = NULL; mdb->num_pixfmts = 0;
    }

    if (NULL != mdb->syncs) {
    	for (i = 0; i < mdb->num_syncs; i ++) {
	    if (NULL != mdb->syncs[i]) {
	    
	    	DisposeObject(mdb->syncs[i]);
		mdb->syncs[i] = NULL;
	    }
	}
	FreeMem(mdb->syncs,   sizeof (Object *) * mdb->num_syncs);
	mdb->syncs = NULL; mdb->num_syncs = 0;
    }
    
    if (NULL != mdb->orig_mode_bm.bm) {
	free_mode_bm(&mdb->orig_mode_bm, cl);
    }
    
    if (NULL != mdb->checked_mode_bm.bm) {
	free_mode_bm(&mdb->checked_mode_bm, cl);
    }
    ReleaseSemaphore(&mdb->sema);
    return;
}

/* Initializes default tagarray. in numtags the TAG_MORE is not accounted for,
   so the array must be of size NUM_TAGS + 1
*/
static VOID init_def_tags(struct TagItem *tags, ULONG numtags)
{
    ULONG i;
    for (i = 0; i < numtags; i ++) {
	tags[i].ti_Tag = TAG_IGNORE;
	tags[i].ti_Data = 0UL;
    }
    tags[i].ti_Tag  = TAG_MORE;
    tags[i].ti_Data = 0UL;
    
    return;
}
static BOOL register_modes(Class *cl, Object *o, struct TagItem *modetags)
{
    struct TagItem *tag, *tstate;
    struct HIDDGraphicsData *data;
    
    DECLARE_ATTRCHECK(sync);
//    DECLARE_ATTRCHECK(pixfmt);
    
    struct mode_db *mdb;
    
    HIDDT_PixelFormat pixfmt_data;
    struct sync_data sync_data;
    
    struct TagItem def_sync_tags[num_Hidd_Sync_Attrs     + 1];
    struct TagItem def_pixfmt_tags[num_Hidd_PixFmt_Attrs + 1];
    
    ULONG numpfs = 0,numsyncs	= 0;
    ULONG pfidx = 0, syncidx = 0;
    
    data = INST_DATA(cl, o);
    mdb = &data->mdb;
    InitSemaphore(&mdb->sema);
    
    memset(&pixfmt_data, 0, sizeof (pixfmt_data));
    memset(&sync_data,   0, sizeof (sync_data));
    
    init_def_tags(def_sync_tags,	num_Hidd_Sync_Attrs);
    init_def_tags(def_pixfmt_tags,	num_Hidd_PixFmt_Attrs);
    
    /* First we need to calculate how much memory we are to allocate by counting supplied
       pixel formats and syncs */
    
    for (tstate = modetags; (tag = NextTagItem((const struct TagItem **)&tstate));) {
	ULONG idx;
	if (IS_GFX_ATTR(tag->ti_Tag, idx)) {
	    switch (idx) {
		case aoHidd_Gfx_PixFmtTags:	numpfs	 ++;    break;
		case aoHidd_Gfx_SyncTags:	numsyncs ++;	break;
		default: break;
	    }
	}
    }
    
    if (0 == numpfs || 0 == numsyncs) {
	kprintf("!!! WE MUST AT LEAST HAVE ONE PIXFMT AND ONE SYNC IN Gfx::RegisterModes() !!!\n");
    }

    ObtainSemaphore(&mdb->sema);
    
    /* Allocate memory for mode db */
    if (!alloc_mode_db(&data->mdb, numsyncs, numpfs, cl))
	goto failure;
    
    
    for (tstate = modetags; (tag = NextTagItem((const struct TagItem **)&tstate));) {
	/* Look for Gfx, PixFmt and Sync tags */
	ULONG idx;
	
	if (IS_GFX_ATTR(tag->ti_Tag, idx)) {
	    switch (idx) {
		case aoHidd_Gfx_PixFmtTags:
		    def_pixfmt_tags[num_Hidd_PixFmt_Attrs].ti_Data = tag->ti_Data;
		    mdb->pixfmts[pfidx] = HIDD_Gfx_RegisterPixFmt(o, def_pixfmt_tags);
		    if (NULL == mdb->pixfmts[pfidx]) {
			kprintf("!!! UNABLE TO CREATE PIXFMT OBJECT IN Gfx::RegisterModes() !!!\n");
			goto failure;
		    }
		    pfidx ++;
		    break;
		    
		case aoHidd_Gfx_SyncTags:
		    def_sync_tags[num_Hidd_Sync_Attrs].ti_Data = tag->ti_Data;
		    if (!parse_sync_tags(def_sync_tags
			    , &sync_data
			    , ATTRCHECK(sync)
			    , CSD(cl) )) {
			kprintf("!!! ERROR PARSING SYNC TAGS IN Gfx::RegisterModes() !!!\n");
			goto failure;
		    } else {
			mdb->syncs[syncidx] = create_and_init_object(CSD(cl)->syncclass
			    , (UBYTE *)&sync_data
			    , sizeof (sync_data)
			    , CSD(cl) );
			if (NULL == mdb->syncs[syncidx]) {
			    kprintf("!!! UNABLE TO CREATE PIXFMT OBJECT IN Gfx::RegisterModes() !!!\n");
			    goto failure;
			}
			syncidx ++;
		    }
		    break;
	    }
	} else if (IS_SYNC_ATTR(tag->ti_Tag, idx)) {
	    if (idx >= num_Hidd_Sync_Attrs) {
		kprintf("!!! UNKNOWN SYNC ATTR IN Gfx::New(): %d !!!\n", idx);
	    } else {
		def_sync_tags[idx].ti_Tag  = tag->ti_Tag;
		def_sync_tags[idx].ti_Data = tag->ti_Data;
	    }
	} else if (IS_PIXFMT_ATTR(tag->ti_Tag, idx)) {
	    if (idx >= num_Hidd_PixFmt_Attrs) {
		kprintf("!!! UNKNOWN PIXFMT ATTR IN Gfx::New(): %d !!!\n", idx);
	    } else {
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


struct modequery {
    struct mode_db *mdb;
    ULONG minwidth;
    ULONG maxwidth;
    ULONG minheight;
    ULONG maxheight;
    HIDDT_StdPixFmt *stdpfs;
    ULONG numfound;
    ULONG pfidx;
    ULONG syncidx;
    BOOL dims_ok;
    BOOL stdpfs_ok;
    BOOL check_ok;
    Class *cl;
};



/* This is a recursive function that looks for valid modes */
static HIDDT_ModeID *querymode(struct modequery *mq) {
    HIDDT_ModeID *modeids;
    register Object *pf;
    register Object *sync;
    BOOL mode_ok = FALSE;
    Class *cl = mq->cl;
    
    mq->dims_ok	  = FALSE;
    mq->stdpfs_ok = FALSE;
    mq->check_ok  = FALSE;
    
    /* Look at the supplied idx */
    if (mq->pfidx >= mq->mdb->num_pixfmts) {
	mq->pfidx = 0;
	mq->syncidx ++;
    }
	
    if (mq->syncidx >= mq->mdb->num_syncs) {
	/* We have reached the end of the recursion. Allocate memory and go back */
	modeids = AllocVec(sizeof (HIDDT_ModeID) * (mq->numfound + 1), MEMF_ANY);
	/* Get the end of the array */
	modeids += mq->numfound;
	*modeids -- = vHidd_ModeID_Invalid;
	
	return modeids;
    }
	
    /* Get the pf and sync objects */
    pf   = mq->mdb->pixfmts[mq->pfidx];
    sync = mq->mdb->syncs[mq->syncidx];
    

    /* Check that the mode is really usable */
    if (is_valid_mode(&mq->mdb->checked_mode_bm, mq->syncidx, mq->pfidx)) {
	mq->check_ok = TRUE;
    
    
	/* See if this mode matches the criterias set */
	
	if (	SD(sync)->hdisp  >= mq->minwidth
	     && SD(sync)->hdisp  <= mq->maxwidth
	     && SD(sync)->vdisp >= mq->minheight
	     && SD(sync)->vdisp <= mq->maxheight	) {
	     
	     
	    mq->dims_ok = TRUE;

	    if (NULL != mq->stdpfs) {
		register HIDDT_StdPixFmt *stdpf = mq->stdpfs;
		while (*stdpf) {
		    if (*stdpf == PF(pf)->stdpixfmt) {
		       	mq->stdpfs_ok  = TRUE;
		    }
		    stdpf ++;
		}
	    } else {
	    	mq->stdpfs_ok = TRUE;
	    }
	}
    }
    
    
    if (mq->dims_ok && mq->stdpfs_ok && mq->check_ok) {
	mode_ok = TRUE;
	mq->numfound ++;
    }
    
    modeids = querymode(mq);

    if (NULL == modeids)
	return NULL;
	
    if (mode_ok) {
	/* The mode is OK. Add it to the list */
	*modeids -- = COMPUTE_HIDD_MODEID(mq->syncidx, mq->pfidx);
    }
    
    return modeids;
	
}

/*** HIDDGfx::QueryModeIDs() ********************************************/
static HIDDT_ModeID *hiddgfx_querymodeids(Class *cl, Object *o, struct pHidd_Gfx_QueryModeIDs *msg)
{
    struct TagItem *tag, *tstate;
    
    HIDDT_ModeID *modeids;
    struct HIDDGraphicsData *data;
    struct mode_db *mdb;
    
    struct modequery mq = {
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
    

    data = INST_DATA(cl, o);
    mdb = &data->mdb;
    mq.mdb = mdb;
    mq.cl  = cl;
    
    for (tstate = msg->queryTags; (tag = NextTagItem((const struct TagItem **)&tstate)); ) {
	switch (tag->ti_Tag) {
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

/*** HIDDGfx::ReleaseModes() ********************************************/
static VOID hiddgfx_releasemodeids(Class *cl, Object *o, struct pHidd_Gfx_ReleaseModeIDs *msg)
{
    FreeVec(msg->modeIDs);
}

/*** HIDDGfx::NextModeID *************************************************/
static HIDDT_ModeID hiddgfx_nextmodeid(Class *cl, Object *o, struct pHidd_Gfx_NextModeID *msg)
{
    struct HIDDGraphicsData *data;
    struct mode_db *mdb;
    ULONG syncidx, pfidx;
    HIDDT_ModeID return_id = vHidd_ModeID_Invalid;
    BOOL found = FALSE;
    
    data = INST_DATA(cl, o);
    mdb = &data->mdb;

    ObtainSemaphoreShared(&mdb->sema);    
    if (vHidd_ModeID_Invalid == msg->modeID) {
	pfidx	= 0;
	syncidx = 0;	
    } else {
	pfidx 	= MODEID_TO_PFIDX( msg->modeID );
	syncidx	= MODEID_TO_SYNCIDX( msg->modeID );

	/* Increament one from the last call */
	pfidx ++;
	if (pfidx >= mdb->num_pixfmts) {
	    pfidx = 0;
	    syncidx ++;
	}
    }
    
    /* Search for a new mode. We only accept valid modes */
    for (; syncidx < mdb->num_syncs; syncidx ++) {
	/* We only return valid modes */
	for (; pfidx < mdb->num_pixfmts; pfidx ++) {
	    if (is_valid_mode(&mdb->checked_mode_bm, syncidx, pfidx)) {
		found = TRUE;
		break;
	    }
	}
	if (found)
	    break;
    }
    if (found) {
	return_id = COMPUTE_HIDD_MODEID(syncidx, pfidx);
	*msg->syncPtr	= mdb->syncs[syncidx];
	*msg->pixFmtPtr	= mdb->pixfmts[pfidx];
    } else {
	*msg->syncPtr = *msg->pixFmtPtr = NULL;
    }
	
    ReleaseSemaphore(&mdb->sema);
    
    return return_id;
}

/*** HiddGfx::GetMode() **************************************************/

static BOOL hiddgfx_getmode(Class *cl, Object *o, struct pHidd_Gfx_GetMode *msg)
{
    ULONG pfidx, syncidx;
    struct HIDDGraphicsData *data;
    struct mode_db *mdb;
    BOOL ok = FALSE;
    
    data = INST_DATA(cl, o);
    mdb = &data->mdb;
    
    pfidx	= MODEID_TO_PFIDX(msg->modeID);
    syncidx	= MODEID_TO_SYNCIDX(msg->modeID);
    
    ObtainSemaphoreShared(&mdb->sema);
    
    if (! (pfidx >= mdb->num_pixfmts || syncidx >= mdb->num_syncs) ) {
	if (is_valid_mode(&mdb->checked_mode_bm, syncidx, pfidx)) {
	    ok = TRUE;
	    *msg->syncPtr	= mdb->syncs[syncidx];
	    *msg->pixFmtPtr	= mdb->pixfmts[pfidx];
	}
    }
    ReleaseSemaphore(&mdb->sema);
    
    if (!ok) {
	*msg->syncPtr = *msg->pixFmtPtr = NULL;
    }
    return ok;
}

/*** HIDDGfx::SetCursor() ********************************************/
static BOOL hiddgfx_setcursor(Class *cl, Object *o, struct pHidd_Gfx_SetCursor *msg)
{
    /* Parse the cursor tags */
    struct TagItem *tag, *tstate;
    
    struct HIDDGraphicsData *data;
    BOOL on;
    LONG xpos, ypos;
    Object *bitmap;
    BOOL update_curs = FALSE;
    BOOL bitmap_changed = FALSE;
    Object *new_backup = NULL;
        
    data = INST_DATA(cl, o);
    on 		= data->curs_on;
    bitmap	= data->curs_bm;
    xpos	= data->curs_x;
    ypos	= data->curs_y; 
    
    
    for (tstate = msg->cursorTags; (tag = NextTagItem((const struct TagItem **)&tstate)); ) {
	switch (tag->ti_Tag) {
	    case tHidd_Cursor_BitMap:
		bitmap = (Object *)tag->ti_Data;
		break;

	    case tHidd_Cursor_XPos:
		xpos = (LONG)tag->ti_Data;
		break;

	    case tHidd_Cursor_YPos:
		ypos = (LONG)tag->ti_Data;
		break;

	    case tHidd_Cursor_On:
		on = (BOOL)tag->ti_Data;
		break;
	 }
    }
    
    /* Look for changes */
    if (bitmap != data->curs_bm) {
	/* Bitmap changed. */
	if (NULL == bitmap) {
	    /* Erase the old cursor */
	    on = FALSE;
	    update_curs = TRUE;
	    bitmap_changed = TRUE;
	    
	    
	} else {
	    IPTR curs_width, curs_height, curs_depth;
	    IPTR mode_width, mode_height, mode_depth;
	    Object *curs_pf, *mode_sync,* mode_pf;
	    struct TagItem bmtags[] = {
		{ aHidd_BitMap_Displayable,	FALSE	},
		{ aHidd_BitMap_Width,		0	},
		{ aHidd_BitMap_Height,		0	},
		{ aHidd_BitMap_PixFmt,		0	},
		{ TAG_DONE, 0UL }
	    };
	    
	    GetAttr(bitmap, aHidd_BitMap_Width,	 &curs_width);
	    GetAttr(bitmap, aHidd_BitMap_Height, &curs_height);
	    GetAttr(bitmap, aHidd_BitMap_PixFmt, (IPTR *)&curs_pf);
	    
	    GetAttr(curs_pf, aHidd_PixFmt_Depth, &curs_depth);
	    
	    HIDD_Gfx_GetMode(o, data->curmode, &mode_sync, &mode_pf);
	    GetAttr(mode_sync, aHidd_Sync_HDisp, &mode_width);
	    GetAttr(mode_sync, aHidd_Sync_VDisp, &mode_height);
	    GetAttr(mode_pf, aHidd_PixFmt_Depth, &mode_depth);

	    /* Disallow very large cursors, and cursors with higher
	       depth than the framebuffer bitmap */
	    if (    ( curs_width  > (mode_width  / 2) )
		 || ( curs_height > (mode_height / 2) )
		 || ( curs_depth  > mode_depth) ) {
		 return FALSE;
	    }
	    
	    /* Create new backup bitmap */
	    bmtags[1].ti_Data = curs_width;
	    bmtags[2].ti_Data = curs_height;
	    bmtags[3].ti_Data = (IPTR)curs_pf;
	    new_backup = NewObject(
		  curs_depth >= 8 ? CSD(cl)->chunkybmclass : CSD(cl)->planarbmclass
		, NULL
		, bmtags
	    );
	    
	    if (NULL == new_backup)
		return FALSE;

	    if (on)
		update_curs = TRUE;
		
	    bitmap_changed = TRUE;
	}
    }
    
    if (on != data->curs_on) {
	update_curs = TRUE;
    }
    
    if (xpos != data->curs_x || ypos != data->curs_y) {
	update_curs = TRUE;
    }
    
    /* Do we want to erase the old cursor ? */
    if (data->curs_bm && data->curs_on && update_curs) {
	draw_cursor(data, FALSE, CSD(cl));
	if (bitmap_changed) {
	    if (NULL != data->curs_backup) {
		DisposeObject(data->curs_backup);
		data->curs_backup = NULL;
	    }
	}
    }

    data->curs_bm	= bitmap;
    data->curs_on	= on;
    data->curs_x	= xpos;
    data->curs_y	= ypos;
    
    if (data->curs_bm && data->curs_on && update_curs) {
	if (bitmap_changed) {
	    data->curs_backup = new_backup;
	}
	draw_cursor(data, TRUE, CSD(cl));
    }
    
    return TRUE;
}

/*** HIDDGfx::RegisterPixFmt() ********************************************/

static Object *hiddgfx_registerpixfmt(Class *cl, Object *o, struct pHidd_Gfx_RegisterPixFmt *msg)
{
    HIDDT_PixelFormat cmp_pf;
    struct HIDDGraphicsData *data;
    
    struct pfnode *pfnode;
    
    Object *retpf = NULL;
    
    data = INST_DATA(cl, o);
    
    if (!parse_pixfmt_tags(msg->pixFmtTags, &cmp_pf, 0, CSD(cl))) {
    	kprintf("!!! FAILED PARSING TAGS IN Gfx::RegisterPixFmt() !!!\n");
	return FALSE;
    }
    
    ObtainSemaphoreShared(&data->pfsema);
    pfnode = find_pixfmt(&data->pflist, &cmp_pf, CSD(cl));
    ReleaseSemaphore(&data->pfsema);
    
    if (NULL == pfnode) {
    	retpf = find_stdpixfmt(&cmp_pf, CSD(cl));
    } else {
    	retpf = pfnode->pixfmt;
	/* Increase pf refcount */
	pfnode->refcount ++;
    }
    

    if (NULL == retpf) {
    	struct pfnode *newnode;
    	/* Could not find an alike pf, Create a new pfdb node  */
	
	newnode = AllocMem(sizeof (struct pfnode), MEMF_ANY);
	if (NULL != newnode) {
	    
	    /* Since we pass NULL as the taglist below, the PixFmt class will just create a dummy pixfmt */
	    retpf = NewObject(CSD(cl)->pixfmtclass, NULL, NULL);
	    if (NULL != retpf) {
	    	newnode->pixfmt = retpf;
		
		/* We have one user */
		newnode->refcount = 1;
		
		/* Initialize the pixfmt object the "ugly" way */
		memcpy(retpf, &cmp_pf, sizeof (HIDDT_PixelFormat));
		
#define PF(x) ((HIDDT_PixelFormat *)x)    
/*
kprintf("(%d, %d, %d, %d), (%x, %x, %x, %x), %d, %d, %d, %d\n"
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
	    
	    } else {
	    	FreeMem(newnode, sizeof (struct pfnode));
	    
	    }
	}
    }
    
    return retpf;
    
    
}


static VOID hiddgfx_releasepixfmt(Class *cl, Object *o, struct pHidd_Gfx_ReleasePixFmt *msg)
{
    struct HIDDGraphicsData *data;
    
    struct objectnode *n, *safe;

/*    kprintf("release_pixfmt\n");
*/    
    data = INST_DATA(cl, o);
    
    /* Go through the pixfmt list trying to find the object */
    ObtainSemaphore(&data->pfsema);
      
    ForeachNodeSafe(&data->pflist, n, safe) {
    	if (msg->pixFmt == n->object) {
	    n->refcount --;
	    if (0 == n->refcount) {
	    
	    	/* Remove the node */
		Remove((struct Node *)n);
		
		/* Dispose the pixel format */
		DisposeObject(n->object);
		
		/* Free the node */
		FreeMem(n, sizeof (struct objectnode));
		
	    }
	    break;
	}
    }
    
    ReleaseSemaphore(&data->pfsema);
}

/** Gfx::CheckMode() *******************************************/

/*
    This is a method which should be implemented by the subclasses.
    It should look at the supplied pixfmt and sync and see if it is valid.
    This will handle any special cases that are not covered by the 
    pixfmts/syncs supplied in HIDD_Gfx_RegisterModes().
    For example some advanced modes, like 1024x768x24 @ 90 might not be available
    because of lack of bandwidth in the gfx hw.
*/
static BOOL hiddgfx_checkmode(Class *cl, Object *o, struct pHidd_Gfx_CheckMode *msg)
{
    /* As a default we allways return TRUE, ie. the mode is OK */
    return TRUE;
}

/*** Gfx::GetPixFmt() **********************************************/


static Object *hiddgfx_getpixfmt(Class *cl, Object *o, struct pHidd_Gfx_GetPixFmt *msg) 
{
    Object *fmt;
    
    if (!IS_REAL_STDPIXFMT(msg->stdPixFmt)) {
    	kprintf("!!! Illegal pixel format passed to Gfx::GetPixFmt(): %d\n", msg->stdPixFmt);
	return NULL;
    }  else  {
    	fmt = CSD(cl)->std_pixfmts[REAL_STDPIXFMT_IDX(msg->stdPixFmt)];
    }

    return fmt;
}

/*************************** Classes *****************************/

#undef OOPBase
#undef SysBase
#undef UtilityBase

#define SysBase     (csd->sysbase)
#define OOPBase     (csd->oopbase)
#define UtilityBase (csd->utilitybase)

#define NUM_ROOT_METHODS	3
#define NUM_GFXHIDD_METHODS	13

Class *init_gfxhiddclass (struct class_static_data *csd)
{
    Class *cl = NULL;
    
    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] = {
        {(IPTR (*)())root_new,    	     	moRoot_New	},
        {(IPTR (*)())root_dispose,         	moRoot_Dispose	},
        {(IPTR (*)())root_get,         		moRoot_Get	},
	{ NULL, 0UL }
    };
    
    struct MethodDescr gfxhidd_descr[NUM_GFXHIDD_METHODS + 1] = 
    {
        {(IPTR (*)())hiddgfx_newgc,         	moHidd_Gfx_NewGC		},
        {(IPTR (*)())hiddgfx_disposegc,     	moHidd_Gfx_DisposeGC		},
        {(IPTR (*)())hiddgfx_newbitmap,     	moHidd_Gfx_NewBitMap		},
        {(IPTR (*)())hiddgfx_disposebitmap, 	moHidd_Gfx_DisposeBitMap	},
        {(IPTR (*)())hiddgfx_querymodeids, 	moHidd_Gfx_QueryModeIDs		},
        {(IPTR (*)())hiddgfx_releasemodeids, 	moHidd_Gfx_ReleaseModeIDs	},
	{(IPTR (*)())hiddgfx_checkmode, 	moHidd_Gfx_CheckMode		},
	{(IPTR (*)())hiddgfx_nextmodeid,	moHidd_Gfx_NextModeID		},
	{(IPTR (*)())hiddgfx_getmode, 		moHidd_Gfx_GetMode		},
        {(IPTR (*)())hiddgfx_registerpixfmt, 	moHidd_Gfx_RegisterPixFmt	},
        {(IPTR (*)())hiddgfx_releasepixfmt, 	moHidd_Gfx_ReleasePixFmt	},
	{(IPTR (*)())hiddgfx_getpixfmt, 	moHidd_Gfx_GetPixFmt		},
	{(IPTR (*)())hiddgfx_setcursor, 	moHidd_Gfx_SetCursor		},
        {NULL, 0UL}
    };
    
    
    struct InterfaceDescr ifdescr[] =
    {
        {root_descr,    IID_Root, 	NUM_ROOT_METHODS},
        {gfxhidd_descr, IID_Hidd_Gfx,	NUM_GFXHIDD_METHODS},
        {NULL, NULL, 0}
    };
    
    AttrBase MetaAttrBase = GetAttrBase(IID_Meta);
        
    struct TagItem tags[] =
    {
     /*   { aMeta_SuperID,                (IPTR)CLID_Hidd},*/
        { aMeta_SuperID,                (IPTR)CLID_Root},

        { aMeta_InterfaceDescr,         (IPTR)ifdescr},
        { aMeta_ID,                     (IPTR)CLID_Hidd_Gfx},
        { aMeta_InstSize,               (IPTR)sizeof (struct HIDDGraphicsData) },
        {TAG_DONE, 0UL}
    };
    

    EnterFunc(bug("init_gfxhiddclass(csd=%p)\n", csd));

    cl = NewObject(NULL, CLID_HiddMeta, tags);
    D(bug("Class=%p\n", cl));
	    if(NULL == cl)
    	goto failexit;
        
    cl->UserData = (APTR)csd;
    AddClass(cl);
    
    if (!ObtainAttrBases(attrbases))
	goto failexit;

    csd->bitmapclass = init_bitmapclass(csd);
    if (NULL == csd->bitmapclass)
    	goto failexit;
	
    csd->gcclass = init_gcclass(csd);
    if (NULL == csd->gcclass)
    	goto failexit;
            
	    
    csd->planarbmclass = init_planarbmclass(csd);
    if (NULL == csd->planarbmclass)
    	goto failexit;

    csd->chunkybmclass = init_chunkybmclass(csd);
    if (NULL == csd->chunkybmclass)
    	goto failexit;
	
    csd->pixfmtclass = init_pixfmtclass(csd);
    if (NULL == csd->pixfmtclass)
    	goto failexit;
	
    csd->syncclass = init_syncclass(csd);
    if (NULL == csd->syncclass)
    	goto failexit;
	
    csd->colormapclass = init_colormapclass(csd);
    if (NULL == csd->colormapclass)
    	goto failexit;

D(bug("Creating std pixelfmts\n"));
    if (!create_std_pixfmts(csd))
    	goto failexit;
D(bug("Pixfmts created\n"));

    /* Get two methodis required for direct method execution */
#if USE_FAST_PUTPIXEL
    csd->putpixel_mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutPixel);
#endif
#if USE_FAST_GETPIXEL
    csd->getpixel_mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_GetPixel);
#endif
#if USE_FAST_DRAWPIXEL
    csd->drawpixel_mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_DrawPixel);
#endif

    ReturnPtr("init_gfxhiddclass", Class *, cl);
    
failexit:
    free_gfxhiddclass(csd);
    ReturnPtr("init_gfxhiddclass", Class *, NULL);
   
}


VOID free_gfxhiddclass(struct class_static_data *csd)
{
    EnterFunc(bug("free_gfxhiddclass(csd=%p)\n", csd));
    if(NULL != csd) {
	
	free_colormapclass(csd);
	free_syncclass(csd);
	free_pixfmtclass(csd);
	free_chunkybmclass(csd);
	free_planarbmclass(csd);
        free_gcclass(csd);
        free_bitmapclass(csd);

	delete_std_pixfmts(csd);
        
	if (NULL != csd->gfxhiddclass) {
    	    RemoveClass(csd->gfxhiddclass);
	    DisposeObject((Object *) csd->gfxhiddclass);
    	    csd->gfxhiddclass = NULL;
	}
	
	ReleaseAttrBases(attrbases);
    }
    
    ReturnVoid("free_gfxhiddclass");
}

const HIDDT_PixelFormat stdpfs[] = 
{
    {   
	  24, 24, 3
#if AROS_BIG_ENDIAN	  
	, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000
	, 8, 16, 24, 0
#else
	, 0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000
	, 24, 16, 8, 0
#endif	
	, 0, 0
	, vHidd_StdPixFmt_RGB24
	, PF_GRAPHTYPE(TrueColor, Chunky)
    }, {
	  16, 16, 2
	, 0x0000F800, 0x000007E0, 0x0000001E, 0x00000000
	, 16, 21, 27, 0
	, 0, 0
	, vHidd_StdPixFmt_RGB16
	, PF_GRAPHTYPE(TrueColor, Chunky)
    }, {
	  32, 32, 4
#if AROS_BIG_ENDIAN
	, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000
	, 8, 16, 24, 0
#else	
	, 0x0000FF00, 0x00FF0000, 0xFF000000, 0x000000FF
	, 16, 8, 0, 24
#endif
	, vHidd_StdPixFmt_ARGB32
	, PF_GRAPHTYPE(TrueColor, Chunky)
    }, {
	  32, 32, 4
#if AROS_BIG_ENDIAN	  
	, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF
	, 0, 8, 16, 24
#else
	, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
	, 24, 16, 8, 0
#endif	
	, 0, 0
	, vHidd_StdPixFmt_RGBA32
	, PF_GRAPHTYPE(TrueColor, Chunky)
    }, {
	  8, 8, 1
	, 0UL, 0UL, 0UL, 0UL
	, 0, 0, 0, 0
	, 0x000000FF, 0
	, vHidd_StdPixFmt_LUT8
	, PF_GRAPHTYPE(Palette, Chunky)
    }, {
    	  1, 1, 1
	, 0UL, 0UL, 0UL, 0UL
	, 0, 0, 0, 0
	, 0x0000000F, 0
	, vHidd_StdPixFmt_Plane
	, PF_GRAPHTYPE(Palette, Planar)
    }
    
};

static BOOL create_std_pixfmts(struct class_static_data *csd)
{
    ULONG i;
    
    memset(csd->std_pixfmts, 0, sizeof (Object *) * num_Hidd_StdPixFmt);
    
    for (i = 0; i < num_Hidd_StdPixFmt; i ++) {
    	csd->std_pixfmts[i] = create_and_init_object(csd->pixfmtclass
		    , (UBYTE *)&stdpfs[i],  sizeof (stdpfs[i]), csd);
	if (NULL == csd->std_pixfmts[i]) {
	    kprintf("FAILED TO CREATE PIXEL FORMAT %d\n", i);
	    delete_std_pixfmts(csd);
	    ReturnBool("create_stdpixfmts", FALSE);
	}
    }
    ReturnBool("create_stdpixfmts", TRUE);

}


static VOID delete_std_pixfmts(struct class_static_data *csd)
{
    ULONG i;
    
    for (i = 0; i < num_Hidd_StdPixFmt; i ++) {
    
        if (NULL != csd->std_pixfmts[i]) {
	    DisposeObject(csd->std_pixfmts[i]);
	    csd->std_pixfmts[i] = NULL;
	}
    }
}


static VOID free_objectlist(struct List *list, BOOL disposeobjects, struct class_static_data *csd)
{
    struct objectnode *n, *safe;
    ForeachNodeSafe(list, n, safe) {
    	Remove(( struct Node *)n);
	
	if (NULL != n->object && disposeobjects) {
	    DisposeObject(n->object);
	}
	
	FreeMem(n, sizeof (struct objectnode));
    }

}


static inline BOOL cmp_pfs(HIDDT_PixelFormat *tmppf, HIDDT_PixelFormat *dbpf)
{
    if (    dbpf->depth == tmppf->depth 
	 && HIDD_PF_COLMODEL(dbpf) == HIDD_PF_COLMODEL(tmppf)) {
    /* The pixfmts are very alike, check all attrs */
	     
	tmppf->stdpixfmt = ((HIDDT_PixelFormat *)dbpf)->stdpixfmt;
	     
	if (0 == memcmp(tmppf, dbpf, sizeof (HIDDT_PixelFormat))) {
	    return TRUE;
	}
    }
    return FALSE;
}

/*
     Matches the supplied pixelformat against all standard pixelformats
     to see if there allready exsts a pixelformat object for this pixelformat
*/

static Object *find_stdpixfmt(HIDDT_PixelFormat *tofind
	, struct class_static_data *csd)
{
    Object *retpf = NULL;
    Object *stdpf;
    ULONG i;
    
    for (i = 0; i < num_Hidd_StdPixFmt; i ++) {
    	stdpf = csd->std_pixfmts[i];
	
	if (cmp_pfs(tofind, (HIDDT_PixelFormat *)stdpf)) {
	    retpf =  stdpf;
	}
	
    }
    return retpf;
}


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
BOOL parse_sync_tags(struct TagItem *tags, struct sync_data *data, ULONG ATTRCHECK(sync), struct class_static_data *csd)
{
    BOOL ok = FALSE;
    IPTR attrs[num_Hidd_Sync_Attrs];
    
    if (0 != ParseAttrs(tags, attrs, num_Hidd_Sync_Attrs, &ATTRCHECK(sync), HiddSyncAttrBase)) {
	kprintf("!!! parse_sync_tags: ERROR PARSING ATTRS !!!\n");
	return FALSE;
    }

    /* Check that we have all attrs */
    if (GOT_SYNC_ATTR(PixelTime)) {
	data->pixtime = attrs[SYAO(PixelTime)];
    } else if (GOT_SYNC_ATTR(PixelClock)) {
	DOUBLE pixclock, pixtime;
		    
	/* Compute the pixtime */
	pixclock =(DOUBLE)attrs[SYAO(PixelClock)];
	pixtime = 1 / pixclock;
	pixtime *= 1000000000000;
	data->pixtime = (ULONG)pixtime;
	
    } else {
	kprintf("!!! MISSING PIXELTIME/CLOCK ATTR !!!\n");
	return FALSE;
    }
    
    /* Check that we have HDisp and VDisp */
    if (SYNC_DISP_AF != (SYNC_DISP_AF & ATTRCHECK(sync))) {
	    kprintf("!!! MISSING HDISP OR VDISP ATTR !!!\n");
    } else {
	data->hdisp = attrs[SYAO(HDisp)];
	data->vdisp = attrs[SYAO(VDisp)];

	/* Test that the user has not supplied both X11 style and fbdev style attrs */
	if ( (LINUXFB_SYNC_AF & ATTRCHECK(sync)) != 0 && (X11_SYNC_AF & ATTRCHECK(sync)) != 0 ) {
	    kprintf("!!! BOTH LINUXFB-STYLE AND X11-STYLE ATTRS WERE SUPPLIED !!!\n");
	    kprintf("!!! YOU MAY ONLY SUPPLY ONE OF THEM !!!\n");
	} else {
	    
	    /* Test that we have all attrs of either the X11 style or the Linux FB style */
	    if ((LINUXFB_SYNC_AF & ATTRCHECK(sync)) == LINUXFB_SYNC_AF) {
		/* Set the data struct */
		data->left_margin	= attrs[SYAO(LeftMargin)];
		data->right_margin	= attrs[SYAO(RightMargin)];
		data->hsync_length	= attrs[SYAO(HSyncLength)];
		
		data->upper_margin	= attrs[SYAO(UpperMargin)];
		data->lower_margin	= attrs[SYAO(LowerMargin)];
		data->vsync_length	= attrs[SYAO(VSyncLength)];
		ok = TRUE;
		
	    } else if ((X11_SYNC_AF & ATTRCHECK(sync)) == X11_SYNC_AF) {
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
	    } else {
		kprintf("!!! UNSUFFICIENT ATTRS PASSED TO parse_sync_tags: %x !!!\n", ATTRCHECK(sync));
	    }
	}
    }
    
    return ok;
}

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
			  
#define PF_PALETTE_AF ( PFAF(CLUTMask) | PFAF(CLUTShift) )
		       
#define PFAO(x) (aoHidd_PixFmt_ ## x)
  
BOOL parse_pixfmt_tags(struct TagItem *tags, HIDDT_PixelFormat *pf, ULONG ATTRCHECK(pixfmt), struct class_static_data *csd)
{
    IPTR attrs[num_Hidd_PixFmt_Attrs];
    
    
    if (0 != ParseAttrs(tags, attrs, num_Hidd_PixFmt_Attrs, &ATTRCHECK(pixfmt), HiddPixFmtAttrBase)) {
	kprintf("!!! parse_pixfmt_tags: ERROR PARSING TAGS THROUGH ParseAttrs !!!\n");
	return FALSE;
    }

    if (PF_COMMON_AF != (PF_COMMON_AF & ATTRCHECK(pixfmt))) {
	kprintf("!!! parse_pixfmt_tags: Missing PixFmt attributes passed to parse_pixfmt_tags(): %x !!!\n", ATTRCHECK(pixfmt));
	return FALSE;
    }
    
    /* Set the common attributes */
    pf->depth		= attrs[PFAO(Depth)];
    pf->size		= attrs[PFAO(BitsPerPixel)];
    pf->bytes_per_pixel	= attrs[PFAO(BytesPerPixel)];
    
    SET_PF_COLMODEL(  pf, attrs[PFAO(ColorModel)]);
    SET_PF_BITMAPTYPE(pf, attrs[PFAO(BitMapType)]);
    
    /* Set the colormodel specific stuff */
    switch (HIDD_PF_COLMODEL(pf)) {
    	case vHidd_ColorModel_TrueColor:
	    /* Check that we got all the truecolor describing stuff */
	    if (PF_TRUECOLOR_AF != (PF_TRUECOLOR_AF & ATTRCHECK(pixfmt))) {
		 kprintf("!!! Unsufficient true color format describing attrs to pixfmt in parse_pixfmt_tags() !!!\n");
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
	    if ( PF_PALETTE_AF != (PF_PALETTE_AF & ATTRCHECK(pixfmt))) {
		 kprintf("!!! Unsufficient palette format describing attrs to pixfmt in parse_pixfmt_tags() !!!\n");
		 return FALSE;
	    }
	    
	    /* set palette stuff */
	    pf->clut_mask	= attrs[PFAO(CLUTMask)];
	    pf->clut_shift	= attrs[PFAO(CLUTShift)];
	    break;
    } /* shift (colormodel) */
    
    return TRUE;
}

/* 
    Create an empty object and initialize it the "ugly" way. This only works with
    CLID_Hidd_PixFmt and CLID_Hidd_Sync classes
*/
static Object *create_and_init_object(Class *cl, UBYTE *data, ULONG datasize, struct class_static_data *csd)
{
    Object *o;
			
    o = NewObject(cl, NULL, NULL);
    if (NULL == o) {
	kprintf("!!! UNABLE TO CREATE OBJECT IN create_and_init_object() !!!\n");
	return NULL;
    }
	    
    memcpy(o, data, datasize);
    
    return o;
}
	

static struct pfnode *find_pixfmt(struct MinList *pflist
	, HIDDT_PixelFormat *tofind
	, struct class_static_data *csd)
{
    Object *retpf = NULL;
    HIDDT_PixelFormat *db_pf;
    struct pfnode *n;
    
    /* Go through the pixel format list to see if a similar pf allready exists */
    ForeachNode(pflist, n) {
    	db_pf = (HIDDT_PixelFormat *)n->pixfmt;
	if (cmp_pfs(tofind, db_pf)) {
	    retpf = (Object *)db_pf;
	    break;
	}
    }
    
    if (NULL != retpf)
    	return n;
	
    return NULL;
}

static VOID draw_cursor(struct HIDDGraphicsData *data, BOOL draw, struct class_static_data *csd)
{
    IPTR width, height;
    IPTR fb_width, fb_height;
    ULONG x, y;
    LONG w2end;
    LONG h2end;
    
    struct TagItem gctags[] = {
	{ aHidd_GC_DrawMode, vHidd_GC_DrawMode_Copy	},
	{ TAG_DONE, 0UL }
    };
    
    GetAttr(data->curs_bm, aHidd_BitMap_Width,  &width);
    GetAttr(data->curs_bm, aHidd_BitMap_Height, &height);
    
    GetAttr(data->framebuffer, aHidd_BitMap_Width,  &fb_width);
    GetAttr(data->framebuffer, aHidd_BitMap_Height, &fb_height);
    
    /* Do some clipping */
    x = data->curs_x;
    y = data->curs_y;
    
    w2end = fb_width  - 1 - data->curs_x;
    h2end = fb_height - 1 - data->curs_y;
    
    if (w2end <= 0 || h2end <= 0) /* Cursor outside framebuffer */
	return;

    if (w2end < width)
	width -= (width - w2end);
	
    if (h2end < height)
	height -= (height - h2end);
    
    SetAttrs(data->gc, gctags);
    
    if (draw) {
	/* Backup under the new cursor image */
	HIDD_BM_CopyBox(data->framebuffer
	    , data->gc
	    , data->curs_x
	    , data->curs_y
	    , data->curs_backup
	    , 0, 0
	    , width, height
	);
	
	/* Render the cursor image */
	HIDD_BM_CopyBox(data->curs_bm
	    , data->gc
	    , 0, 0
	    , data->framebuffer
	    , data->curs_x, data->curs_y
	    , width, height
	);
	
    } else {
	/* Erase the old cursor image */
	HIDD_BM_CopyBox(data->framebuffer
	    , data->gc
	    , 0, 0
	    , data->framebuffer
	    , data->curs_x
	    , data->curs_y
	    , width, height
	);
    }
    return;
}


#undef OOPBase
#define OOPBase (OCLASS(OCLASS(OCLASS(o)))->UserData)

/*********** Stubs for private methods **********************/
Object *HIDD_Gfx_RegisterPixFmt(Object *o, struct TagItem *pixFmtTags)
{
   static MethodID mid = 0;
   
   struct pHidd_Gfx_RegisterPixFmt p;
   
   if (!mid) mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_RegisterPixFmt);
   
   p.mID = mid;
   
   p.pixFmtTags = pixFmtTags;
   
   return (Object *)DoMethod(o, (Msg)&p);
   
}


VOID HIDD_Gfx_ReleasePixFmt(Object *o, Object *pixFmt)
{
   static MethodID mid = 0;
   
   struct pHidd_Gfx_ReleasePixFmt p;
   
   if (!mid) mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_ReleasePixFmt);
   
   p.mID = mid;
   
   p.pixFmt = pixFmt;
   
   
   
   DoMethod(o, (Msg)&p);


}




