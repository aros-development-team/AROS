#define AROS_ALMOST_COMPATIBLE 1
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/arossupport.h>
#include <proto/utility.h>
#include <proto/graphics.h>

#include <exec/lists.h>
#include <exec/memory.h>

#include <graphics/displayinfo.h>
#include <graphics/monitor.h>

#include <oop/oop.h>

#include <hidd/graphics.h>

#include <stdio.h>
#include <string.h>

#include "graphics_intern.h"
#include "graphics_internal.h"

#define NOTNULLMASK 0x10000000

#define MAJOR_ID_SHIFT 26
#define MINOR_ID_SHIFT 20

#define NUM2MAJORID(num) ((num)  << MAJOR_ID_SHIFT)
#define MAJORID2NUM(modeid) ( ((modeid) & ~NOTNULLMASK) >> MAJOR_ID_SHIFT)

#define NUM2MINORID(num) ((num)  << MINOR_ID_SHIFT)
#define MINORID2NUM(modeid) ( ((modeid) & ~NOTNULLMASK) >> MINOR_ID_SHIFT)



/* This macro assures that a modeid is never 0 by setting the MSB to 1.
   This is usefull because FindDisplayInfo just returns the modeid,
   and FidDisplayInfo returning 0 indicates failure
*/
   
#define GENERATE_MODEID(majoridx, minoridx)	\
	(NUM2MAJORID(majoridx) | NUM2MINORID(minoridx) | NOTNULLMASK)

struct displayinfo_item  {

    ULONG	modeid;
    Object	*gfxmode;
    struct MonitorSpec mspc;
};

struct displayinfo_db {
    struct displayinfo_item *dispitems;
    struct SignalSemaphore sema;
    ULONG current_major_num;
    ULONG	nummodes;
};

static ULONG compute_major_modeid(Object *gfxmode, struct displayinfo_db *db, struct GfxBase *GfxBase)
{
    ULONG id;
    id = NUM2MAJORID(db->current_major_num);
    
    db->current_major_num ++;
    return id;
    
}

VOID destroy_dispinfo_db(APTR dispinfo_db, struct GfxBase *GfxBase)
{
    struct displayinfo_db *db;
    
    
    db = (struct displayinfo_db *)dispinfo_db;
    
    ObtainSemaphore(&db->sema);
    
    if (NULL != db->dispitems) {
	ULONG i;
	
	for (i = 0; i < db->nummodes; i ++) {
	    if (NULL != db->dispitems[i].gfxmode) {
	    	DisposeObject(db->dispitems[i].gfxmode);
		db->dispitems[i].gfxmode = NULL;
	    }
	}
	
	FreeMem(db->dispitems, sizeof (*db->dispitems) * db->nummodes);
    }
    
    ReleaseSemaphore(&db->sema);
    FreeMem(db, sizeof (*db));
    
}

APTR build_dispinfo_db(struct List *gfxmodes, struct GfxBase *GfxBase)
{
    struct displayinfo_db *db;
    struct ModeNode *mnode;
    
    
    db = AllocMem(sizeof (struct displayinfo_db), MEMF_PUBLIC | MEMF_CLEAR);
    if (NULL != db) {
	
    	InitSemaphore(&db->sema);
    
    	/* Go through the gfxmode DB, counting all elemets*/
    	db->nummodes = 0;
	db->current_major_num = 0;
    	ForeachNode(gfxmodes, mnode) {
    	    db->nummodes ++;
   	}
    
    	/* Allocate a table to hold all the modes */
    	db->dispitems = AllocMem(sizeof (*db->dispitems) * db->nummodes, MEMF_PUBLIC | MEMF_CLEAR);
    	if (NULL != db->dispitems) {
	
	    ULONG modeidx = 0;
	    
	    ForeachNode(gfxmodes, mnode) {
	    
	    	db->dispitems[modeidx].gfxmode = mnode->gfxMode;
		db->dispitems[modeidx].modeid  = GENERATE_MODEID(compute_major_modeid(mnode->gfxMode, db, GfxBase), 0);
		
		modeidx ++;
	    }
	    return (APTR)db;
	}
	
	destroy_dispinfo_db(db, GfxBase);
    
    }
    
    return NULL;

}


ULONG driver_NextDisplayInfo(ULONG lastid, struct GfxBase *GfxBase)
{
    struct displayinfo_db *db;
    
    ULONG id = INVALID_ID;
    
    kprintf("NextDisplayInfo(lastid=%x)\n", lastid);
    
    db = (struct displayinfo_db *)SDD(GfxBase)->dispinfo_db;
    
    ObtainSemaphoreShared(&db->sema);

    if (INVALID_ID == lastid) {
	if  (0 != db->nummodes)
	     id = GENERATE_MODEID(0, 0);	
    } else {
    	ULONG majoridx, minoridx;
	Object *gm;
	ULONG numpfs;
	
	majoridx = MAJORID2NUM(lastid);
	minoridx = MINORID2NUM(lastid);
	
	kprintf("GETTING GFXMODE %d %d\n", majoridx, minoridx);
	gm = db->dispitems[majoridx].gfxmode;
	GetAttr(gm, aHidd_GfxMode_NumPixFmts, &numpfs);
	
	/* Increase pixfmt idx */
	minoridx ++;
	
	/* More pixfmts available for this gfxmode ? */
	if (minoridx >= numpfs) {
	
	    /* No. Got to next gfxmode */
	    majoridx ++;
	    
	    /* More gfxmodes availavle ? */
	    if (majoridx < db->nummodes) {
	    	/* Yes. compute new id */
		id = GENERATE_MODEID(majoridx, minoridx);
	    }
	    
	} else {
	    /* Yes. compute new id */
	    id = GENERATE_MODEID(majoridx, minoridx);
	}
    }
    
    ReleaseSemaphore(&db->sema);
    
    kprintf("NextDisplayInfo returning %x\n", id);
    
    return id;
    
}


DisplayInfoHandle driver_FindDisplayInfo(ULONG id, struct GfxBase *GfxBase)
{
    struct displayinfo_db *db;
    DisplayInfoHandle ret = NULL;
    ULONG majoridx, minoridx;
    
    kprintf("FindDisplayInfo(id=%x)\n", id);
    
    /* Check for the NOTNULLMASK */
    if ((id & NOTNULLMASK) != NOTNULLMASK) {
    	kprintf("!!! NO AROS MODEID IN FindDisplayInfo() !!!\n");
    	return NULL;
    }
    db = (struct displayinfo_db *)SDD(GfxBase)->dispinfo_db;
    
    
    
    majoridx = MAJORID2NUM(id);
    minoridx = MINORID2NUM(id);
    
    /* Here we obly check if the mode is really available */
    
    
    ObtainSemaphoreShared(&db->sema);
    
    if (majoridx < db->nummodes) {
    	Object *gm;
	ULONG numpfs;
	
	gm = db->dispitems[majoridx].gfxmode;
	
	GetAttr(gm, aHidd_GfxMode_NumPixFmts, &numpfs);
	
	/* Check that the pixfmt is available */
	if (minoridx < numpfs) {
	
	    /* We simply return the modeid, and use this to lookup in GetDisplayInfoData() */
	    ret = (DisplayInfoHandle)GENERATE_MODEID(majoridx, minoridx);
	}
	
    }
    
    ReleaseSemaphore(&db->sema);

kprintf("FindDisplayInfo() returning %x\n", ret);    
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
    { DTAG_NAME, sizeof(struct NameInfo), 		"NameInfo"	},
    { DTAG_VEC,  sizeof(struct VecInfo), 		"VecInfo"	},
    { PRIV_DTAG_QHDR,  sizeof(struct QueryHeader), 	"QueryHeader"	}
    
};


#define DTAG_TO_IDX(dtag) (((dtag) & 0x0000F000) >> 12)

static inline BOOL check_sizes(ULONG tagid, ULONG size)
{
    ULONG idx;
    const struct size_check *sc;
    
    idx = DTAG_TO_IDX(tagid);
    
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

#define DLONGSZ (sizeof (ULONG) * 2)

static ULONG compute_numbits(HIDDT_Pixel mask)
{
    ULONG i;
    ULONG numbits = 0;
    
    for (i = 0; i <= 31; i ++) {
    	if (mask & (1L << i))
	    numbits ++;
    }
    
    return numbits;
}
ULONG driver_GetDisplayInfoData(DisplayInfoHandle handle, UBYTE *buf, ULONG size, ULONG tagid, ULONG id2, struct GfxBase *GfxBase)
{
    struct displayinfo_item *ditem;
    struct QueryHeader *qh;
    ULONG structsize;
    Object *gm, *pf;
    
    
    ULONG modeid, majoridx, minoridx;
    struct displayinfo_db *db;
    
    
    
    if (NULL == handle) {
	if (INVALID_ID != id2) {
	
	    /* Check that id2 is a valid modeid */
	    handle = FindDisplayInfo(id2);
	    
	} else {
	    kprintf("!!! INVALID MODE ID IN GetDisplayInfoData()\n");
	    return 0;
	}
    }
    
    if (NULL == handle) {
	kprintf("!!! COULD NOT GET HANDLE IN GetDisplayInfoData()\n");
	return 0;
    }
    
    
    modeid = (ULONG)handle;
    majoridx = MAJORID2NUM(modeid);
    minoridx = MINORID2NUM(modeid);
    
    /* Look up the display info item */
    db = (struct displayinfo_db *)SDD(GfxBase)->dispinfo_db;
    
    
ObtainSemaphoreShared(&db->sema);
    ditem = &db->dispitems[majoridx];
    
    kprintf("GetDisplayInfoData(handle=%d, major=%d, minor=%d, modeid=%x, tagid=%x)\n"
    	, (ULONG)handle, majoridx, minoridx, ditem->modeid, tagid);
	
    
    /* Build the queryheader */
    if (!check_sizes(tagid, size)) return 0;
    
    memset(buf, 0, size);
    
    /* Fill in the queryheader */
    qh = (struct QueryHeader *)buf;
    qh->StructID  = tagid;
    qh->DisplayID = ditem->modeid;
    qh->SkipID	  = TAG_SKIP;
    
    structsize = size_checks[DTAG_TO_IDX(tagid)].struct_size;
    
    qh->Length	  = (structsize + (DLONGSZ - 1)) / DLONGSZ;
    
    gm = ditem->gfxmode;
    
    /* Get the belonging pixelformat */
    pf = HIDD_GM_LookupPixFmt(gm, minoridx);
    
    switch (tagid) {
    	case DTAG_DISP: {
	    struct DisplayInfo *di;
	    HIDDT_Pixel redmask, greenmask, bluemask;
	    
	    di = (struct DisplayInfo *)buf;
	    di->NotAvailable = FALSE;
	    
	    /* Set the propertyflags */
	    
	    di->PropertyFlags = DIPF_IS_FOREIGN;
	    
	    /* We simulate AGA. This field is really obsolete */
	    di->PaletteRange = 4096;

	    /* Compute red green and blue bits */
	    GetAttr(pf, aHidd_PixFmt_RedMask,	&redmask);
	    GetAttr(pf, aHidd_PixFmt_GreenMask, &greenmask);
	    GetAttr(pf, aHidd_PixFmt_BlueMask,	&bluemask);
	    
	    di->RedBits	  = compute_numbits(redmask);
	    di->GreenBits = compute_numbits(greenmask);
	    di->BlueBits  = compute_numbits(bluemask);
	    
/*	    
	    di->Resolution.x = ?;
	    di->Resolution.y = ?;
	    di->PixelSpeed = ?;
	    di->NumStdSprites = ?
	    di->SpriteResolution.x = ?;
	    di->SpriteResolution.y = ?;
*/	    
	    
	    break; }
	    
	case DTAG_DIMS: {
	    struct DimensionInfo *di;
	    ULONG depth, width, height;
	    
	    GetAttr(pf, aHidd_PixFmt_Depth,  &depth);
	    GetAttr(gm, aHidd_GfxMode_Width,  &width);
	    GetAttr(gm, aHidd_GfxMode_Height, &height);
	    
	    di = (struct DimensionInfo *)buf;
	    di->MaxDepth = depth;
	    
	    di->MinRasterWidth	= width;
	    di->MinRasterHeight	= height;
	    di->MaxRasterWidth	= width;
	    di->MaxRasterHeight	= height;
	    
	    di->Nominal.MinX	= 0;
	    di->Nominal.MinY	= 0;
	    di->Nominal.MaxX	= width  - 1;
	    di->Nominal.MaxY	= height - 1;
	  
	  
#warning What about the OSCAN stuff ??
	    di->MaxOScan	= di->Nominal;
	    di->VideoOScan	= di->Nominal;
	    di->TxtOScan	= di->Nominal;
	    di->StdOScan	= di->Nominal;

/* 
	    di->MaxOScan.MinX	= di->Nominal.MinX;
	    di->MaxOScan.MinY	= di->Nominal.MinY;
	    di->MaxOScan.MaxX	= di->Nominal.MaxX;
	    di->MaxOScan.MaxY	= di->Nominal.MaxY;


	    di->VideoOScan.MinX	= di->Nominal.MinX;
	    di->VideoOScan.MinY	= di->Nominal.MinY;
	    di->VideoOScan.MaxX	= di->Nominal.MaxX;
	    di->VideoOScan.MaxY	= di->Nominal.MaxY;

	    di->TxtOScan.MinX	= di->Nominal.MinX;
	    di->TxtOScan.MinY	= di->Nominal.MinY;
	    di->TxtOScan.MaxX	= di->Nominal.MaxX;
	    di->TxtOScan.MaxY	= di->Nominal.MaxY;

	    di->StdOScan.MinX	= di->Nominal.MinX;
	    di->StdOScan.MinY	= di->Nominal.MinY;
	    di->StdOScan.MaxX	= di->Nominal.MaxX;
	    di->StdOScan.MaxY	= di->Nominal.MaxY;
*/	    
	    break; }
	    
	case DTAG_MNTR: {
	    struct MonitorInfo *mi;
	    struct MonitorSpec *mspc;
	    
	    
	    mi = (struct MonitorInfo *)buf;
	    
	    mspc = &ditem->mspc;
	    
	    /*
	    mi->ViewPosition.X = ?;
	    mi->ViewPosition.Y = ?;
	    mi->ViewResolution.X = ?;
	    mi->ViewResolution.Y = ?;
	    mi->ViewPositionRange.MinX = ?;
	    mi->ViewPositionRange.MinY = ?;
	    mi->ViewPositionRange.MaxX = ?;
	    mi->ViewPositionRange.MaxY = ?;
	    mi->TotalRows = ?;
	    mi->TotalColorClocks = ?;
	    mi->MinRow = ?;
	    mi->MouseTicks.X = ?;
	    mi->MouseTicks.Y = ?;
	    mi->DefaultViewPosition.X = ?;
	    mi->DefaultViewPosition.Y = ?;
	    */
	    
	    
	    mi->Mspc = mspc;
	    mi->PreferredModeID = ditem->modeid;
	    mi->Compatibility = MCOMPAT_NOBODY;
	    
	    /* Fill info into the monitorspec. It is by default set to all 0s */
	    mspc->ms_Node.xln_Pred = mspc->ms_Node.xln_Succ = NULL;
	    mspc->ms_Node.xln_Type = MONITOR_SPEC_TYPE;
	    mspc->ms_Node.xln_Name = "AROS.monitor";
	    mspc->total_rows = mi->TotalRows;
	    mspc->total_colorclocks = mi->TotalColorClocks;
	    mspc->min_row = mi->MinRow;
	    
	    /* Waht to put in here ? */
	    mspc->ms_Flags = 0;
	    break; }
	    
	case DTAG_NAME: {
	    struct NameInfo *ni;
	    ULONG depth, width, height;
	    
	    GetAttr(pf, aHidd_PixFmt_Depth,  &depth);
	    GetAttr(gm, aHidd_GfxMode_Width,  &width);
	    GetAttr(gm, aHidd_GfxMode_Height, &height);
	    
	    ni = (struct NameInfo *)buf;
	    
	    snprintf(ni->Name, DISPLAYNAMELEN
	    	, "AROS: %dx%dx%d"
		, width, height, depth
	    );
	    break; }
	    
	default:
	    kprintf("!!! UNKNOWN TAGID IN CALL TO GetDisplayInfoData() !!!\n");
	    break;
    	
    }
    
ReleaseSemaphore(&db->sema);    

kprintf("GDID: %d\n", structsize);
    return structsize;
}


ULONG driver_GetVPModeID(struct ViewPort *vp, struct GfxBase *GfxBase)
{
    ULONG modeid;
    kprintf(" GetVPModeID returning %x\n", vp->ColorMap->VPModeID);
    modeid = vp->ColorMap->VPModeID;
    
    kprintf("RETURNING\n");
    return modeid;
    
}


ULONG driver_BestModeIDA(struct TagItem *tags, struct GfxBase *GfxBase)
{
/*    ULONG dipf_must_have = 0;
    ULONG dipf_must_not_have = 0;
    */
    struct ViewPort *vp = NULL;
    UWORD nominal_width		= 640
    	, nominal_height	= 200
	, desired_width		= 640
	, desired_height	= 200;
    UBYTE depth = 1;
    ULONG monitorid, sourceid;
    UBYTE redbits	= 4
    	, greenbits	= 4
	, bluebits	= 4;
	
    struct displayinfo_db *db;
    ULONG i;
    
    ULONG found_id = INVALID_ID;

/*    ULONG maxdepth = 0;
    ULONG maxwidth = 0, maxheight = 0;
    UBYTE maxrb = 0, maxgb = 0, maxbb = 0;
*/    
    /* First try to get viewport */
    vp		= (struct ViewPort *)GetTagData(BIDTAG_ViewPort, (IPTR)NULL, tags);
    sourceid	= GetTagData(BIDTAG_SourceID, INVALID_ID, tags);
    
    if (NULL != vp) {
    	/* Set some new default values */
	nominal_width  = desired_width  = vp->DWidth;
	nominal_height = desired_height = vp->DHeight;
	
	if (NULL != vp->RasInfo->BitMap) {
	    depth = vp->RasInfo->BitMap->Depth;
	} else {
	    kprintf("!!! Passing viewport with NULL vp->RasInfo->BitMap to BestModeIDA() !!!\n");
	}
    }
    
    if (INVALID_ID != sourceid) {
#warning Fix this

/* I do not understand what the docs state about this */
	
    }
    
    /* OK, now we try to search for a mode that has the supplied charateristics */
    
    db = (struct displayinfo_db *)SDD(GfxBase)->dispinfo_db;
    
    ObtainSemaphoreShared(&db->sema);
    
    for (i = 0; i < db->nummodes; i ++) {
    	struct displayinfo_item *ditem;
	Object *gm, *pf;
	ULONG numpfs;
	
	
	ULONG redmask, greenmask, bluemask;
	ULONG gm_depth, gm_width, gm_height;
	ULONG pfno;
	

	ditem = &db->dispitems[i];
	gm = ditem->gfxmode;
	
	GetAttr(gm, aHidd_GfxMode_NumPixFmts,	&numpfs);
	for (pfno = 0; pfno < numpfs; pfno ++) {
	
	    pf = HIDD_GM_LookupPixFmt(gm, pfno);
	
	    GetAttr(pf, aHidd_PixFmt_RedMask,	&redmask);
	    GetAttr(pf, aHidd_PixFmt_GreenMask,	&greenmask);
	    GetAttr(pf, aHidd_PixFmt_BlueMask,	&bluemask);
	
	    GetAttr(pf, aHidd_PixFmt_Depth,		&gm_depth);
	    GetAttr(gm, aHidd_GfxMode_Width,	&gm_width);
	    GetAttr(gm, aHidd_GfxMode_Height,	&gm_height);
kprintf("BestModeIDA: checking mode %d, id %x, (%dx%dx%d)\n"
	, i, ditem->modeid, gm_width, gm_height, gm_depth );
	
	    if (    compute_numbits(redmask)   >= redbits
	         && compute_numbits(greenmask) >= greenbits
	         && compute_numbits(bluemask)  >= bluebits
	         && gm_depth  >= depth
	         && gm_width  >= desired_width
	         && gm_height >= desired_height) {
	     

#warning Fix this 	    
	    	/* We return the first modeid that fulfill the criterias.
	 	      Instead we should find the mode that has:
		           - largest possible depth.
			   - width/height that are nearest above or equal the desired ones
	    	*/
		found_id = ditem->modeid;
		break;
	    }
	     
	} /* for (each pf in gfxmode) */
    }
    
    ReleaseSemaphore(&db->sema);
    
    
    return found_id;
}

