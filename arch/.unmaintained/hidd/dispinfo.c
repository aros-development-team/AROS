#define AROS_ALMOST_COMPATIBLE 1
#include <proto/exec.h>
#include <proto/oop.h>

#include <exec/lists.h>
#include <exec/memory.h>

#include <graphics/displayinfo.h>

#include <oop/oop.h>

#include "graphics_intern.h"
#include "graphics_internal.h"

#define NUM2ID(num) ((num)  << 16)
#define ID2NUM(modeid) ( (modeid) >> 16)

struct displayinfo_item  {

    ULONG	modeid;
    Object	*gfxmode;
    ULONG	nummodes;
};

struct displayinfo_db {
    struct displayinfo_item *dispitems;
    struct SignalSemaphore sema;
    ULONG currentnum;
};

static ULONG compute_modeid(Object *gfxmode, struct displayinfo_db *db, struct GfxBase *GfxBase)
{
    ULONG id;
    id = NUM2ID(db->currentnum);
    db->currentnum ++;
    return id;
    
}

APTR build_dispinfo_db(struct List *gfxmodes, struct GfxBase *GfxBase)
{
    struct displayinfo_db *db;
    struct ModeNode *mnode;
    
    
    db = AllocMem(sizeof (struct displayinfo_db), MEMF_PUBLIC);
    if (NULL != db) {
	
    	InitSemaphore(&db->sema);
    
    	/* Go through the gfxmode DB, counting all elemets*/
    	db->nummodes = 0;
    	ForeachNode(gfxmodes, mnode) {
    	    db->nummodes ++
   	}
    
    	/* Allocate a table to hold all the modes */
    	db->dispitems = AllocMem(sizeof (*db->dispitems) * db->nummodes, MEMF_PUBLIC | MEMF_CLEAR);
    	if (NULL != db->dispitems) {
	
	    ULONG modeidx = 0;
	    
	    ForeachNode(gfxmodes, mnode) {
	    
	    	db->dispitems[modeidx].gfxmode = mnode->gfxMode;
		db->dispitems[modeidx].modeid  = compute_mode_id(mnode->gfxMode, db, GfxBase);
		
		modeidx ++;
	    }
	    return (APTR)db;
	}
	
	destroy_dispinfo_db(db, GfxBase);
    
    }
    
    return NULL;

}

VOID destroy_dispinfo_db(APTR dispinfo_db)
{
    struct diplayinfo_db *db;
    
    db = (struct displayinfo_db *)dispinfo_db;
    ObtainSemaphore(&db->sema);
    
    if (NULL != db->dispitems)
	FreeMem(db->dispitems, sizeof (*db->dispitems) * db->nummodes));
    
    ReleaseSemaphore(&db->sema);
    FreeMem(db, sizeof (*db));
    
}

ULONG driver_NextDisplayInfo(ULONG lastid, struct GfxBase *GfxBase)
{
    struct displayinfo_db *db;
    struct displayinfo_node *dinode;
    
    ULONG id = IVALID_ID;
    
    displayinfo_db = (struct displayinfo_db *)SDD(GfxBase)->dispinfo_db);
    
    ObtainSemaphoreShared(&db->sema);

    if (INVALID_ID == lastid) {
	if  (0 != db->nummodes)
	     id = db->dispitems[0].modeid;	
    } else {
    	ULONG idx;
	
	idx = ID2NUM(lastid);
	idx ++;
	if (idx < db->nummodes)
	    id = db->dispitems[idx].modeid;
    }
    
    ReleaseSemaphore(&db->sema);
    
    return id;
    
}


DisplayInfoHandle driver_FindDisplayInfo(ULONG id, struct GfxBase *GfxBase)
{
    struct displayinfo_db *db;
    DisplayInfoHandle ret = NULL;
    ULONG idx;
    
    sb = (struct displayinfo_db *)SDD(GfxBase)->dispinfo_db);
    idx = ID2NUM(id);
    
    ObtainSemaphoreShared(&db->sema);
    
    if (idx < db->nummodes)
    	ret = &db->dispitems[idx];
    
    ReleaseSemaphore(&db->sema);
    
    return ret;
    
}

#define NUM_SIZECHECKS

#define PRIV_DTAG_QHDR 0x80005000
const struct size_check {
    ULONG struct_id;
    ULONG struct_size;
    STRPTR struct_name;
} size_checks[] = {
    { DTAG_DISP, sizeof(struct DisplayInfo),		"DisplayInfo"	},
    { DTAG_DIMS, sizeof(struct DimensionInfo),		"DimensionInfo"	},
    { DTAG_MNTR, sizeof(struct MonitorInfo),		"MonitorInfo"	},
    { DTAG_NAME, sizeof(struct NameInfo), 		"NameInfo"	}
    { DTAG_VEC,  sizeof(struct VecInfo), 		"VecInfo"	}
    { PRIV_DTAG_QHDR,  sizeof(struct QueryHeader), 	"QueryHeader"	}
    
};

static inline BOOL check_sizes(ULONG tagid, ULONG size)
{
    ULONG idx;
    struct size_check *sc;
    
    idx = tagid & 0x0000F000;
    idx >>= 12;
    
    if (idx > 5) {
    	kprintf("!!! INVALID TAGID TO GetDisplayInfoData");
	return FALSE;
    }
    
    sc = &size_checks[idx];
    if (sc->struct_id != tagid) {
    	kprintf("!!! INVALID TAGID TO GetDisplayInfoData");
	return FALSE;
    }
    
    if (sc->struct_size > size) {
    	kprintf("!!! NO SPACE FOR %s IN BUFFER SUPPLIED TO GetDisplayInfoData !!!\n"
		, sc->struct_name);
	return FALSE;
    }
    
    return TRUE;
}

ULONG driver_GetDisplayInfoData(DisplayInfoHandle handle, UBYTE *buf, ULONG size, ULONG tagid, ULONG id2, struct GfxBase *GfxBase)
{
    struct displayinfo_item *ditem;
    struct QueryHeader *qh;
    ditem = (struct displayinfo_item *)handle;
    
    /* Build the queryheader */
    if (!check_sizes(tagid, size)) return 0;
    
    memset(buf, 0, size);
    
    /* Fill in the queryheader */
    qh = (struct QueryHeader *)buf;
    qh->StructID  = tagid;
    qh->DisplayID = ditem->modeid;
    qh->SkipID	  = TAG_SKIP;
    length	  = size;
    
    switch (tagid) {
    	case DTAG_DISP: {
	    struct DisplayInfo *di;
	    
	    di = (struct DisplayInfo *)buf;
	    di->NotAvailable = FALSE;
/*
	    di->PropertyFlags = ?;
	    di->Resolution.x = ?;
	    di->Resolution.y = ?;
	    di->PixelSpeed = ?;
	    di->NumStdSprites = ?
	    di->PaletteRange = ?
	    di->SpriteResolution.x = ?;
	    di->SpriteResolution.y = ?;
	    di->RedBits = ?;
	    di->GreenBits = ?;
	    di->BlueBits = ?;
*/	    
	    
	    break; }
	    
	case DTAG_DIMS: {
	    struct DimensionInfo *di;
	    ULONG depth;
	    
	    GetAttr(ditem->gfxmode, aHidd_GfxMode_Depth,  &depth);
	    GetAttr(ditem->gfxmode, aHidd_GfxMode_Width,  &width);
	    GetAttr(ditem->gfxmode, aHidd_GfxMode_Height, &height);
	    
	    di = (struct DimensionInfo *)buf;
	    di->MaxDepth 0 ;
	
	    
	    break; }
	    
	case DTAG_MNTR:
	    break;
	    
	case DTAG_NAME:
	    break;
	    
	default:
	    kprintf("!!! UNKNOWN TAGID IN CALL TO GetDisplayInfoData() !!!\n");
	    break;
    	
    }
}
