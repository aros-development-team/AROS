/*
    Copyright (C) 2016-2026, The AROS Development Team. All rights reserved.

    Desc: Graphics display-mode enumerator class implementation.
*/

/****************************************************************************************/
#include "gfx_debug.h"

#include <aros/atomic.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <aros/config.h>
#include <cybergraphx/cgxvideo.h>
#include <exec/lists.h>
#include <oop/static_mid.h>
#include <graphics/displayinfo.h>
#include <graphics/view.h>

#include "gfx_intern.h"
#include "gfx_display.h"

#include <string.h>
#include <stddef.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <exec/libraries.h>
#include <exec/memory.h>

#include <utility/tagitem.h>

#include LC_LIBDEFS_FILE

#include <hidd/gfx.h>

/* mode-database helpers */
static BOOL register_modes(OOP_Class *cl, OOP_Object *o, struct TagItem *modetags);
static BOOL alloc_mode_db(struct mode_db *mdb, ULONG numsyncs, ULONG numpfs, OOP_Class *cl);
static VOID free_mode_db(struct mode_db *mdb, OOP_Class *cl);
static struct pixfmt_data *find_pixfmt(HIDDT_PixelFormat *tofind, struct class_static_data *_csd);
BOOL parse_pixfmt_tags(struct TagItem *tags, HIDDT_PixelFormat *pf, ULONG attrcheck, struct class_static_data *_csd);

static inline BOOL cmp_pfs(HIDDT_PixelFormat *tmppf, HIDDT_PixelFormat *dbpf)
{
    /* Just compare everything except stdpixfmt */
    /* Compare flags first (because it's a fast check) */
    if (tmppf->flags != dbpf->flags)
        return FALSE;
    /* If they match, compare the rest of things */
    return !memcmp(tmppf, dbpf, offsetof(HIDDT_PixelFormat, stdpixfmt));
}

#define COMPUTE_HIDD_MODEID(sync, pf)   ( ((sync) << 8) | (pf) )
#define MODEID_TO_SYNCIDX(id) (((id) & 0X0000FF00) >> 8)
#define MODEID_TO_PFIDX(id)   ( (id) & 0x000000FF)

static inline BOOL alloc_mode_bm(struct mode_bm *bm, ULONG numsyncs, ULONG numpfs,
                                 OOP_Class *cl)
{
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
    bm->bpr = WIDTH_TO_BYTES(numpfs);
    
    bm->bm = AllocVec(bm->bpr * numsyncs, MEMF_CLEAR);
    if (NULL == bm->bm)
        return FALSE;
        
    /* We initialize the mode bitmap to all modes valid */
    SetMem(bm->bm, 0xFF, bm->bpr * numsyncs);
    
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

#define MAKE_SYNC(name,clock,hdisp,hstart,hend,htotal,vdisp,vstart,vend,vtotal,descr)   \
    struct TagItem sync_ ## name[]={            \
        { aHidd_Sync_PixelClock,    clock*1000  },  \
        { aHidd_Sync_HDisp,         hdisp   },  \
        { aHidd_Sync_HSyncStart,    hstart  },  \
        { aHidd_Sync_HSyncEnd,      hend    },  \
        { aHidd_Sync_HTotal,        htotal  },  \
        { aHidd_Sync_VDisp,         vdisp   },  \
        { aHidd_Sync_VSyncStart,    vstart  },  \
        { aHidd_Sync_VSyncEnd,      vend    },  \
        { aHidd_Sync_VTotal,        vtotal  },  \
        { aHidd_Sync_Description,       (IPTR)descr},   \
        { TAG_DONE, 0UL }}


/****************************************************************************************/

struct modequery
{
    struct mode_db  *mdb;
    ULONG           minwidth;
    ULONG           maxwidth;
    ULONG           minheight;
    ULONG           maxheight;
    HIDDT_StdPixFmt *stdpfs;
    ULONG           numfound;
    ULONG           pfidx;
    ULONG           syncidx;
    BOOL            dims_ok;
    BOOL            stdpfs_ok;
    BOOL            check_ok;
    OOP_Class       *cl;
};

/****************************************************************************************/

/* This is a recursive function that looks for valid modes */

/****************************************************************************************/

static HIDDT_ModeID *querymode(struct modequery *mq)
{
    HIDDT_ModeID        *modeids;
    register OOP_Object *pf;
    register OOP_Object *sync;
    BOOL                mode_ok = FALSE;
    ULONG               syncidx, pfidx;
    
    mq->dims_ok   = FALSE;
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
        
        if (    SD(sync)->hdisp  >= mq->minwidth
             && SD(sync)->hdisp  <= mq->maxwidth
             && SD(sync)->vdisp >= mq->minheight
             && SD(sync)->vdisp <= mq->maxheight        )
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

static BOOL alloc_mode_db(struct mode_db *mdb, ULONG numsyncs, ULONG numpfs, OOP_Class *cl)
{
    BOOL ok = FALSE;
    
    if (0 == numsyncs || 0 == numpfs)
        return FALSE;
        
    ObtainSemaphore(&mdb->sema);
    /* free_mode_bm() needs this */
    mdb->num_pixfmts    = numpfs;
    mdb->num_syncs      = numsyncs;

    mdb->syncs   = AllocMem(sizeof (OOP_Object *) * numsyncs, MEMF_CLEAR);
    
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
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    ULONG i;
    
    ObtainSemaphore(&mdb->sema);
    
    if (NULL != mdb->pixfmts)
    {
    
        /* Pixelformats are shared objects and never freed */
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

static BOOL register_modes(OOP_Class *cl, OOP_Object *o, struct TagItem *modetags)
{
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct TagItem          *tag, *tstate;
    struct HIDDDMEnumData *data;
    
    MAKE_SYNC(640x480_60,   25174,
         640,  656,  752,  800,
         480,  490,  492,  525,
         "Default:640x480");

    MAKE_SYNC(800x600_56,   36000,  // 36000
         800,  824,  896, 1024,
         600,  601,  603,  625,
         "Default:800x600");

    MAKE_SYNC(1024x768_60, 65000,   //78654=60kHz, 75Hz. 65000=50kHz,62Hz
        1024, 1048, 1184, 1344,
         768,  771,  777,  806,
        "Default:1024x768");

    MAKE_SYNC(1152x864_60, 80000,
        1152, 1216, 1328, 1456,
         864,  870,  875,  916,
        "Default:1152x864");

    MAKE_SYNC(1280x1024_60, 108880,
        1280, 1360, 1496, 1712,
        1024, 1025, 1028, 1060,
        "Default:1280x1024");

    MAKE_SYNC(1600x1200_60, 155982,
        1600, 1632, 1792, 2048,
        1200, 1210, 1218, 1270,
        "Default:1600x1200");

    /* "new" 16:10 modes */

    MAKE_SYNC(1280x800_60, 83530,
        1280, 1344, 1480, 1680,
        800, 801, 804, 828,
        "Default:1280x800");

    MAKE_SYNC(1440x900_60, 106470,
                1440, 1520, 1672, 1904,
                900, 901, 904, 932,
                "Default:1440x900");

    MAKE_SYNC(1680x1050_60, 147140,
                1680, 1784, 1968, 2256,
                1050, 1051, 1054, 1087,
                "Default:1680x1050");

    MAKE_SYNC(1920x1080_60, 173000,
                1920, 2048, 2248, 2576,
                1080, 1083, 1088, 1120,
                "Default:1920x1080");

    MAKE_SYNC(1920x1200_60, 154000,
                1920, 1968, 2000, 2080,
                1200, 1203, 1209, 1235,
                "Default:1920x1200");

    struct mode_db          *mdb;
    
    HIDDT_PixelFormat       pixfmt_data;

    struct TagItem          def_sync_tags[num_Hidd_Sync_Attrs     + 1];
    struct TagItem          def_pixfmt_tags[num_Hidd_PixFmt_Attrs + 1];
    
    ULONG                   numpfs = 0,numsyncs = 0;
    ULONG                   pfidx = 0, syncidx = 0;
    
    struct TagItem temporary_tags[] = {
        { aHidd_DMEnum_SyncTags,   (IPTR)sync_640x480_60   },
        { aHidd_DMEnum_SyncTags,   (IPTR)sync_800x600_56   },
        { aHidd_DMEnum_SyncTags,   (IPTR)sync_1024x768_60  },
        { aHidd_DMEnum_SyncTags,   (IPTR)sync_1152x864_60  },
        { aHidd_DMEnum_SyncTags,   (IPTR)sync_1280x1024_60 },
        { aHidd_DMEnum_SyncTags,   (IPTR)sync_1600x1200_60 },
        { aHidd_DMEnum_SyncTags,   (IPTR)sync_1280x800_60 },
        { aHidd_DMEnum_SyncTags,   (IPTR)sync_1440x900_60 },
        { aHidd_DMEnum_SyncTags,   (IPTR)sync_1680x1050_60 },
        { aHidd_DMEnum_SyncTags,   (IPTR)sync_1920x1080_60 },
        { aHidd_DMEnum_SyncTags,   (IPTR)sync_1920x1200_60 },
        { TAG_MORE, 0UL }
    };

    data = OOP_INST_DATA(cl, o);
    mdb = &data->mdb;
    InitSemaphore(&mdb->sema);

    SetMem(&pixfmt_data, 0, sizeof (pixfmt_data));

    init_def_tags(def_sync_tags,        num_Hidd_Sync_Attrs);
    init_def_tags(def_pixfmt_tags,      num_Hidd_PixFmt_Attrs);

    def_sync_tags[aoHidd_Sync_DMEnumerator].ti_Tag = aHidd_Sync_DMEnumerator;
    def_sync_tags[aoHidd_Sync_DMEnumerator].ti_Data = (IPTR)o;
    
    /* First we need to calculate how much memory we are to allocate by counting supplied
       pixel formats and syncs */
    
    for (tstate = modetags; (tag = NextTagItem(&tstate));)
    {
        ULONG idx;
        
        if (IS_DMENUM_ATTR(tag->ti_Tag, idx))
        {
            switch (idx)
            {
                case aoHidd_DMEnum_PixFmtTags:
                    numpfs++;
                    break;
                    
                case aoHidd_DMEnum_SyncTags:
                    numsyncs ++;
                    break;
                    
                default:
                    break;
            }
        }
    }
    
    if (0 == numpfs)
    {
        D(bug("!!! WE MUST AT LEAST HAVE ONE PIXFMT IN Gfx::RegisterModes() !!!\n"));
    }

    if (0 == numsyncs)
    {
        D(bug("!!! NO SYNC IN Gfx::RegisterModes() !!!\n!!! USING DEFAULT MODES !!!\n"));
        temporary_tags[11].ti_Tag = TAG_MORE;
        temporary_tags[11].ti_Data = (IPTR)modetags;
        modetags = &temporary_tags[0];
        numsyncs = 11;
    }

    ObtainSemaphore(&mdb->sema);
    
    /* Allocate memory for mode db */
    if (!alloc_mode_db(&data->mdb, numsyncs, numpfs, cl))
        goto failure;
    
    
    for (tstate = modetags; (tag = NextTagItem(&tstate));)
    {
        /* Look for Gfx, PixFmt and Sync tags */
        ULONG idx;
        
        if (IS_DMENUM_ATTR(tag->ti_Tag, idx))
        {
            switch (idx)
            {
                case aoHidd_DMEnum_PixFmtTags:
                    def_pixfmt_tags[num_Hidd_PixFmt_Attrs].ti_Data = tag->ti_Data;
                    mdb->pixfmts[pfidx] = DMEnum__Internal__RegisterPixFmt(cl, def_pixfmt_tags);
                    
                    if (NULL == mdb->pixfmts[pfidx])
                    {
                        D(bug("!!! UNABLE TO CREATE PIXFMT OBJECT IN Gfx::RegisterModes() !!!\n"));
                        goto failure;
                    }
                    
                    pfidx ++;
                    break;

                case aoHidd_DMEnum_SyncTags:
                    def_sync_tags[num_Hidd_Sync_Attrs].ti_Data = tag->ti_Data;

                    mdb->syncs[syncidx] = OOP_NewObject(CSD(cl)->syncclass, NULL, def_sync_tags);
                    if (!mdb->syncs[syncidx]) {
                        D(bug("!!! UNABLE TO CREATE SYNC OBJECT IN Gfx::RegisterModes() !!!\n"));
                        goto failure;
                    }

                    syncidx ++;
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

    /*  mode db is freed in dispose */
    ReleaseSemaphore(&mdb->sema);
    
    return FALSE;
}

static struct pixfmt_data *find_pixfmt(HIDDT_PixelFormat *tofind, struct class_static_data *_csd)
{
    struct pixfmt_data  *retpf = NULL;
    HIDDT_PixelFormat   *db_pf;
    struct Node         *n;

    /* Go through the pixel format list to see if a similar pf allready exists */
    ObtainSemaphoreShared(&_csd->pfsema);

    ForeachNode(&_csd->pflist, n)
    {
        db_pf = PIXFMT_OBJ(n);
        DPF(bug("find_pixfmt(): Trying pixelformat 0x%p\n", db_pf));
        DPF(bug("(%d, %d, %d, %d), (%x, %x, %x, %x), %d, %d, %d, %d\n",
              db_pf->red_shift, db_pf->green_shift, db_pf->blue_shift, db_pf->alpha_shift,
              db_pf->red_mask, db_pf->green_mask, db_pf->blue_mask, db_pf->alpha_mask,
              db_pf->bytes_per_pixel, db_pf->size, db_pf->depth, db_pf->stdpixfmt));
        if (cmp_pfs(tofind, db_pf))
        {
            DPF(bug("Match!\n"));
            retpf = (struct pixfmt_data *)db_pf;
            break;
        }
    }

    ReleaseSemaphore(&_csd->pfsema);
    return retpf;
}

OOP_Object *DMEnum__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct HIDDDMEnumData *data = OOP_INST_DATA(cl, o);
        struct TagItem *tstate = msg->attrList;
        struct TagItem *tag;
        BOOL ok;

        InitSemaphore(&data->mdb.sema);
        data->display = NULL;

        while ((tag = NextTagItem(&tstate)))
        {
            ULONG idx;

            Hidd_DMEnum_Switch(tag->ti_Tag, idx)
            {
            case aoHidd_DMEnum_Display:
                data->display = (OOP_Object *)tag->ti_Data;
                break;
            }
        }

        /* Register modes from the supplied PixFmtTags/SyncTags */
        ok = register_modes(cl, o, msg->attrList);
        if (!ok)
        {
            OOP_MethodID dispose_mid = msg->mID - moRoot_New + moRoot_Dispose;
            OOP_CoerceMethod(cl, o, &dispose_mid);
            return NULL;
        }
    }
    return o;
}

VOID DMEnum__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct HIDDDMEnumData *data = OOP_INST_DATA(cl, o);

    free_mode_db(&data->mdb, cl);
    OOP_DoSuperMethod(cl, o, msg);
}

VOID DMEnum__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct HIDDDMEnumData *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    Hidd_DMEnum_Switch (msg->attrID, idx)
    {
    case aoHidd_DMEnum_NumSyncs:
        *msg->storage = data->mdb.num_syncs;
        return;
    case aoHidd_DMEnum_Display:
        *msg->storage = (IPTR)data->display;
        return;
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/*****************************************************************************************

    NAME
        moHidd_Gfx_QueryModeIDs

    SYNOPSIS
        HIDDT_ModeID *OOP_DoMethod(OOP_Object *obj, struct pHidd_DMEnum_QueryModeIDs *msg);

        HIDDT_ModeID *HIDD_Gfx_QueryModeIDs(OOP_Object *gfxHidd, struct TagItem *queryTags);

    LOCATION
        hidd.gfx.driver

    FUNCTION
        Obtain a table of all supported display mode IDs
    
        The returned address points to an array of HIDDT_ModeID containing all ModeIDs
        supported by this driver. The array is terminated with vHidd_ModeID_Invalid.

    INPUTS
        gfxHidd   - A driver object which to query.
        querytags - An optional taglist containing query options. Can be NULL.
                    The following tags are supported:

                    tHidd_GfxMode_MinWidth  (ULONG) - A minimum width of modes you are
                                                      interested in
                    tHidd_GfxMode_MaxWidth  (ULONG) - A maximum width of modes you are
                                                      interested in
                    tHidd_GfxMode_MinHeight (ULONG) - A minimum height of modes you are
                                                      interested in
                    tHidd_GfxMode_MaxHeight (ULONG) - A maximum height of modes you are
                                                      interested in
                    tHidd_GfxMode_PixFmts   (HIDDT_StdPifXmt *) - A pointer to an array
                        of standard pixelformat indexes. If supplied, only mode IDs whose
                        pixelformat numbers match any of given ones will be returned.

    RESULT
        A pointer to an array of ModeIDs or NULL in case of failure

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_Gfx_ReleaseModeIDs, moHidd_Gfx_NextModeID

    INTERNALS

*****************************************************************************************/

HIDDT_ModeID *DMEnum__Hidd_DMEnum__QueryModeIDs(OOP_Class *cl, OOP_Object *o,
                                          struct pHidd_DMEnum_QueryModeIDs *msg)
{
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
    struct TagItem          *tag, *tstate;
    
    HIDDT_ModeID            *modeids;
    struct HIDDDMEnumData *data;
    struct mode_db          *mdb;
    
    struct modequery        mq =
    {
        NULL,           /* mode db (set later)  */
        0, 0xFFFFFFFF,  /* minwidth, maxwidth   */
        0, 0xFFFFFFFF,  /* minheight, maxheight */
        NULL,           /* stdpfs               */
        0,              /* numfound             */
        0, 0,           /* pfidx, syncidx       */
        FALSE, FALSE,   /* dims_ok, stdpfs_ok   */
        FALSE,          /* check_ok             */
        NULL            /* class (set later)    */
        
    };
    

    data = OOP_INST_DATA(cl, o);
    mdb = &data->mdb;
    mq.mdb = mdb;
    mq.cl  = cl;
    
    for (tstate = msg->queryTags; (tag = NextTagItem(&tstate)); )
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

/*****************************************************************************************

    NAME
        moHidd_Gfx_ReleaseModeIDs

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_DMEnum_ReleaseModeIDs *msg);

        VOID HIDD_Gfx_ReleaseModeIDs(OOP_Object *gfxHidd, HIDDT_ModeID *modeIDs);

    LOCATION
        hidd.gfx.driver

    FUNCTION
        Free array of display mode IDs returned by HIDD_Gfx_QueryModeIDs()

    INPUTS
        gfxHidd - A driver object used to obtain the array
        modeIDs - A pointer to an array

    RESULT
        None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_Gfx_QueryModeIDs

    INTERNALS

*****************************************************************************************/

VOID DMEnum__Hidd_DMEnum__ReleaseModeIDs(OOP_Class *cl, OOP_Object *o,
                                   struct pHidd_DMEnum_ReleaseModeIDs *msg)
{
    FreeVec(msg->modeIDs);
}

/*****************************************************************************************

    NAME
        moHidd_Gfx_NextModeID

    SYNOPSIS
        HIDDT_ModeID OOP_DoMethod(OOP_Object *obj, struct pHidd_DMEnum_NextModeID *msg);

        HIDDT_ModeID HIDD_Gfx_NextModeID(OOP_Object *gfxHidd, HIDDT_ModeID modeID,
                                         OOP_Object **syncPtr, OOP_Object **pixFmtPtr);

    LOCATION
        hidd.gfx.driver

    FUNCTION
        Iterate driver's internal display mode database.

    INPUTS
        gfxHidd   - A driver object to query
        modeID    - A previous mode ID or vHidd_ModeID_Invalid for start of the iteration
        syncPtr   - A pointer to a storage where pointer to sync object will be placed
        pixFmtPtr - A pointer to a storage where pointer to pixelformat object will be placed

    RESULT
        Next available mode ID or vHidd_ModeID_Invalid if there are no more display modes.
        If the function returns vHidd_ModeID_Invalid, sync and pixelformat pointers will
        be set to NULL.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_Gfx_GetMode

    INTERNALS

*****************************************************************************************/

HIDDT_ModeID DMEnum__Hidd_DMEnum__NextModeID(OOP_Class *cl, OOP_Object *o,
                                       struct pHidd_DMEnum_NextModeID *msg)
{
    struct HIDDDMEnumData *data;
    struct mode_db          *mdb;
    ULONG                   syncidx, pfidx;
    HIDDT_ModeID            return_id = vHidd_ModeID_Invalid;
    BOOL                    found = FALSE;
    
    data = OOP_INST_DATA(cl, o);
    mdb = &data->mdb;

    ObtainSemaphoreShared(&mdb->sema);
    if (vHidd_ModeID_Invalid == msg->modeID)
    {
        pfidx   = 0;
        syncidx = 0;
    }
    else
    {
        pfidx   = MODEID_TO_PFIDX( msg->modeID );
        syncidx = MODEID_TO_SYNCIDX( msg->modeID );

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
        *msg->syncPtr   = mdb->syncs[syncidx];
        *msg->pixFmtPtr = mdb->pixfmts[pfidx];
    }
    else
    {
        *msg->syncPtr = *msg->pixFmtPtr = NULL;
    }
        
    ReleaseSemaphore(&mdb->sema);
    
    return return_id;
}

/*****************************************************************************************

    NAME
        moHidd_Gfx_GetMode

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *obj, struct pHidd_DMEnum_GetMode *msg);

        BOOL HIDD_Gfx_GetMode(OOP_Object *gfxHidd, HIDDT_ModeID modeID,
                              OOP_Object **syncPtr, OOP_Object **pixFmtPtr);

    LOCATION
        hidd.gfx.driver

    FUNCTION
        Get sync and pixelformat objects for a particular display ModeID.

    INPUTS
        gfxHidd   - pointer to a driver object which this ModeID belongs to
        syncPtr   - pointer to a storage where sync object pointer will be placed
        pixFmtPtr - pointer to a storage where pixelformat object pointer will be placed

    RESULT
        TRUE upon success, FALSE in case of failure (e.g. given mode does not exist in
        driver's internal database). If the function returns FALSE, sync and pixelformat
        pointers will be set to NULL.

    NOTES
        Every display mode is associated with some sync and pixelformat object. If the
        method returns TRUE, object pointers are guaranteed to be valid.

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_Gfx_NextModeID

    INTERNALS

*****************************************************************************************/

BOOL DMEnum__Hidd_DMEnum__GetMode(OOP_Class *cl, OOP_Object *o, struct pHidd_DMEnum_GetMode *msg)
{
    ULONG                   pfidx, syncidx;
    struct HIDDDMEnumData *data;
    struct mode_db          *mdb;
    BOOL                    ok = FALSE;
    
    data = OOP_INST_DATA(cl, o);
    mdb = &data->mdb;
    
    pfidx       = MODEID_TO_PFIDX(msg->modeID);
    syncidx     = MODEID_TO_SYNCIDX(msg->modeID);
    
    ObtainSemaphoreShared(&mdb->sema);
    
    if (! (pfidx >= mdb->num_pixfmts || syncidx >= mdb->num_syncs) )
    {
        if (is_valid_mode(&mdb->checked_mode_bm, syncidx, pfidx))
        {
            ok = TRUE;
            *msg->syncPtr       = mdb->syncs[syncidx];
            *msg->pixFmtPtr     = mdb->pixfmts[pfidx];
        }
    }
    
    ReleaseSemaphore(&mdb->sema);
    
    if (!ok)
    {
        *msg->syncPtr = *msg->pixFmtPtr = NULL;
    }
    
    return ok;
}

/*****************************************************************************************

    NAME
        moHidd_Gfx_SetMode

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *obj, struct pHidd_DMEnum_SetMode *msg);

        BOOL HIDD_Gfx_SetMode(OOP_Object *gfxHidd, OOP_Object *sync);

    LOCATION
        hidd.gfx.driver

    FUNCTION
        Update display mode according to changed sync object

    INPUTS
        gfxHidd - A display driver to operate on
        sync    - A modified sync object pointer

    RESULT
        TRUE if everything went OK and FALSE in case of some error

    NOTES
        This method is used to inform the driver that some external program has changed
        sync data and wants to update the display if needed. It's up to the implementation to
        check that current display is really using this sync (frontmost screen uses this mode).

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        Base class implementation just returns FALSE indicating that this method is
        not supported.

*****************************************************************************************/

BOOL DMEnum__Hidd_DMEnum__SetMode(OOP_Class *cl, OOP_Object *o, struct pHidd_DMEnum_SetMode *msg)
{
    return FALSE;
}

/*
 * The following two methods are private, static, and not virtual.
 * They operate only on static data and don't need object pointer.
 */

OOP_Object *DMEnum__Internal__RegisterPixFmt(OOP_Class *cl, struct TagItem *pixFmtTags)
{
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    HIDDT_PixelFormat       cmp_pf;
    struct class_static_data *data;
    struct pixfmt_data      *retpf = NULL;

    SetMem(&cmp_pf, 0, sizeof(cmp_pf));

    data = CSD(cl);
    if (!parse_pixfmt_tags(pixFmtTags, &cmp_pf, 0, CSD(cl)))
    {
        D(bug("!!! FAILED PARSING TAGS IN Gfx::RegisterPixFmt() !!!\n"));
        return FALSE;
    }

    /*
     * Our alpha-less R8G8B8 pixelformats are defined as having depth
     * and size = 24, not 32 bits. Nevertheless, many hardware reports
     * 32 bits in such cases.
     * In order to avoid confusion we attempt to detect this situation and
     * fix up pixelformat definition. If we don't do it, we get nonstandard
     * pixelformat with no corresponding CGX code, which can misbehave.
     */
    if ((cmp_pf.flags == PF_GRAPHTYPE(TrueColor, Chunky)) &&
        (cmp_pf.bytes_per_pixel == 4) && (cmp_pf.alpha_mask == 0) &&
        (cmp_pf.red_mask << cmp_pf.red_shift == 0xFF000000) &&
        (cmp_pf.green_mask << cmp_pf.green_shift == 0xFF000000) &&
        (cmp_pf.blue_mask << cmp_pf.blue_shift == 0xFF000000))
    {
        DPF(bug("Gfx::RegisterPixFmt(): 4-byte R8G8B8 detected\n"));

        if (cmp_pf.depth > 24)
        {
            DPF(bug("Gfx::RegisterPixFmt(): Fixing up depth %d > 24\n", cmp_pf.depth));
            cmp_pf.depth = 24;
        }

        if (cmp_pf.size > 24)
        {
            DPF(bug("Gfx::RegisterPixFmt(): Fixing up size %d > 24\n", cmp_pf.size));
            cmp_pf.size = 24;
        }
    }

    DPF(bug("Gfx::RegisterPixFmt(): Registering pixelformat:\n"));
    DPF(bug("(%d, %d, %d, %d), (%x, %x, %x, %x), %d, %d, %d, %d\n"
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
          , PF(&cmp_pf)->stdpixfmt));

    retpf = find_pixfmt(&cmp_pf, CSD(cl));

    DPF(bug("Found matching pixelformat: 0x%p\n", retpf));
    if (retpf)
        /* Increase pf refcount */
        AROS_ATOMIC_INC(retpf->refcount);
    else
    {
        /* Could not find an alike pf, Create a new pfdb node  */
        /* Since we pass NULL as the taglist below, the PixFmt class will just create a dummy pixfmt */
        retpf = OOP_NewObject(CSD(cl)->pixfmtclass, NULL, NULL);
        if (retpf) {
            /* We have one user */
            retpf->refcount = 1;

            /* Initialize the pixfmt object the "ugly" way */
            memcpy(retpf, &cmp_pf, sizeof (HIDDT_PixelFormat));

            DPF(bug("(%d, %d, %d, %d), (%x, %x, %x, %x), %d, %d, %d, %d\n"
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
                        , PF(&cmp_pf)->stdpixfmt));

            ObtainSemaphore(&data->pfsema);
            AddTail((struct List *)&data->pflist, (struct Node *)&retpf->node);
            ReleaseSemaphore(&data->pfsema);
            
        }
    }
    DPF(bug("New refcount is %u\n", retpf->refcount));
    return (OOP_Object *)retpf;
}

/* This method doesn't need object pointer, it's static. */

VOID DMEnum__Internal__ReleasePixFmt(OOP_Class *cl, OOP_Object *pf)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct class_static_data *data;
    struct pixfmt_data *pixfmt = (struct pixfmt_data *)pf;

    DPF(bug("release_pixfmt 0x%p\n", pixfmt));

    data = CSD(cl);

    ObtainSemaphore(&data->pfsema);

    /* If refcount is already 0, this object was never registered in the database,
       don't touch it */
    DPF(bug("Old reference count is %u\n", pixfmt->refcount));
    if (pixfmt->refcount) {
        if (--pixfmt->refcount == 0) {
            Remove((struct Node *)&pixfmt->node);
            OOP_DisposeObject((OOP_Object *)pixfmt);
        }
    }

    ReleaseSemaphore(&data->pfsema);
}

/*****************************************************************************************

    NAME
        moHidd_Gfx_CheckMode

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *obj, struct pHidd_DMEnum_CheckMode *msg);

        BOOL HIDD_Gfx_CheckMode(OOP_Object *gfxHidd, HIDDT_ModeID modeID,
                                OOP_Object *sync, OOP_Object *pixFmt);

    LOCATION
        hidd.gfx.driver

    FUNCTION
        Check if given display mode is supported by the driver.

        Normally any resolution (sync) can be used together with any pixelformat. However
        on some hardware there may be exceptions from this rule. In such a case this
        method should be implemented, and check should be performed.

        The information provided by this method is used in order to exclude unsupported
        modes from the database

        Default implementation in the base class just returns TRUE for all supplied values.

        Note that this method can not be used in order to chech that the given mode is
        really present in the database and it really refers to the given sync and
        pixelformat objects. Use HIDD_Gfx_GetMode() for mode ID validation.

    INPUTS
        gfxHidd - A display driver object
        modeID  - A display mode ID
        sync    - A pointer to a sync object associated with this mode
        pixFmt  - A pointer to a pixelformat object associated with this mode

    RESULT
        TRUE if this mode is supported and FALSE if it's not.

    NOTES

    EXAMPLE

    BUGS
        Currently base class does not call this method after driver object creation.
        This needs to be fixed.

    SEE ALSO
        moHidd_Gfx_GetMode

    INTERNALS

*****************************************************************************************/

BOOL DMEnum__Hidd_DMEnum__CheckMode(OOP_Class *cl, OOP_Object *o, struct pHidd_DMEnum_CheckMode *msg)
{
    /* As a default we allways return TRUE, ie. the mode is OK */
    return TRUE;
}

/*****************************************************************************************

    NAME
        moHidd_Gfx_GetPixFmt

    SYNOPSIS
        OOP_Object *OOP_DoMethod(OOP_Object *o, struct pHidd_DMEnum_GetPixFmt *msg);

        OOP_Object *HIDD_Gfx_GetPixFmt(OOP_Object *gfxHidd, HIDDT_StdPixFmt pixFmt);

    LOCATION
        hidd.gfx.driver

    FUNCTION
        Get a standard pixelformat descriptor from internal pixelformats database.

    INPUTS
        gfxHidd - A display driver object
        pixFmt  - An index of pixelformat (one of vHIDD_StdPixFmt_... values)

    RESULT
        A pointer to a pixelformat object or NULL if lookup failed

    NOTES
        Pixelformat objects are stored in a global system-wide database. They are not
        linked with a particular driver in any way and completely sharable between all
        drivers.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        This operation can never fail because all standard pixelformats are registered
        during early system initialization.

*****************************************************************************************/

OOP_Object *DMEnum__Internal__GetPixFmt(OOP_Class *cl, HIDDT_StdPixFmt pixFmt)
{
    OOP_Object *fmt = NULL;

    if (!IS_REAL_STDPIXFMT(pixFmt))
    {
        D(bug("!!! Illegal pixel format passed to DMEnum::GetPixFmt(): %d\n", pixFmt));
    }
    else
    {
        fmt = (OOP_Object *)CSD(cl)->std_pixfmts[REAL_STDPIXFMT_IDX(pixFmt)];
    }

    return fmt;
}

OOP_Object *DMEnum__Hidd_DMEnum__GetPixFmt(OOP_Class *cl, OOP_Object *o, struct pHidd_DMEnum_GetPixFmt *msg)
{
    return DMEnum__Internal__GetPixFmt(cl, msg->stdPixFmt);
}

/*****************************************************************************************

    NAME
        moHidd_Gfx_GetSync

    SYNOPSIS
        OOP_Object *OOP_DoMethod(OOP_Object *obj, struct pHidd_DMEnum_GetSync *msg);

        OOP_Object *HIDD_Gfx_GetSync(OOP_Object *gfxHidd, ULONG num);

    LOCATION
        hidd.gfx.driver

    FUNCTION
        Get a sync object from internal display mode database by index

    INPUTS
        gfxHidd - A display driver object to query
        num     - An index of sync object starting from 0

    RESULT
        A pointer to a sync object or NULL if there's no sync with such index

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

OOP_Object *DMEnum__Hidd_DMEnum__GetSync(OOP_Class *cl, OOP_Object *o, struct pHidd_DMEnum_GetSync *msg)
{
    struct HIDDDMEnumData *data = OOP_INST_DATA(cl, o);
    
    if (msg->num < data->mdb.num_syncs)
        return data->mdb.syncs[msg->num];
    else {
        D(bug("!!! Illegal sync index passed to Gfx::GetSync(): %d\n", msg->num));
        return NULL;
    }
}
