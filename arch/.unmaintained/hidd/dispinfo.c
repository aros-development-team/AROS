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

#include <cybergraphx/cybergraphics.h>

#include <oop/oop.h>

#include <hidd/graphics.h>

#include <stdio.h>
#include <string.h>

#include "graphics_intern.h"
#include "graphics_internal.h"

/* stegerg: check */

/* #define NOTNULLMASK 0x10000000 --> trouble with more than 4 gfxmodes: 4 << 26 = 0x10000000 */
#define NOTNULLMASK 0x80000000

#define MAJOR_ID_MSB   30
#define MAJOR_ID_LSB   26
#define MAJOR_ID_SHIFT MAJOR_ID_LSB
#define MAJOR_ID_MASK  (((1 << (MAJOR_ID_MSB - MAJOR_ID_LSB + 1)) - 1) << MAJOR_ID_LSB)

#define MINOR_ID_MSB   25
#define MINOR_ID_LSB   20
#define MINOR_ID_SHIFT MINOR_ID_LSB
#define MINOR_ID_MASK  (((1 << (MINOR_ID_MSB - MINOR_ID_LSB + 1)) - 1) << MINOR_ID_LSB)

#define NUM2MAJORID(num) ((num)  << MAJOR_ID_SHIFT)
/*#define MAJORID2NUM(modeid) ( ((modeid) & ~NOTNULLMASK) >> MAJOR_ID_SHIFT)*/
#define MAJORID2NUM(modeid) ( ((modeid) & MAJOR_ID_MASK) >> MAJOR_ID_SHIFT)

#define NUM2MINORID(num) ((num)  << MINOR_ID_SHIFT)
/*#define MINORID2NUM(modeid) ( ((modeid) & ~NOTNULLMASK) >> MINOR_ID_SHIFT)*/
#define MINORID2NUM(modeid) ( ((modeid) & MINOR_ID_MASK) >> MINOR_ID_SHIFT)

/* stegerg: end check */


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
#if 1
    /* stegerg */
    id = db->current_major_num;
#else
    id = NUM2MAJORID(db->current_major_num);
#endif

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
		
		/* stegerg: check */
		
		/* was: id = GENERATE_MODEID(majoridx, minoridx); */
		id = GENERATE_MODEID(majoridx, 0);
		
		/* stegerg: end check */
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

    return ret;
    
    
}

#define NUM_SIZECHECKS

#define PRIV_DTAG_QHDR 0x80005000
const struct size_check {
    ULONG struct_id;
    ULONG struct_size;
    STRPTR struct_name;
} size_checks[] = {
    { DTAG_DISP,	sizeof(struct DisplayInfo),	"DisplayInfo"	},
    { DTAG_DIMS,	sizeof(struct DimensionInfo),	"DimensionInfo"	},
    { DTAG_MNTR,	sizeof(struct MonitorInfo),	"MonitorInfo"	},
    { DTAG_NAME,	sizeof(struct NameInfo), 	"NameInfo"	},
    { DTAG_VEC,		sizeof(struct VecInfo), 	"VecInfo"	},
    { PRIV_DTAG_QHDR,	sizeof(struct QueryHeader), 	"QueryHeader"	}
    
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
	    
	    di->PropertyFlags = DIPF_IS_FOREIGN | DIPF_IS_WB;
	    
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
    ULONG monitorid = INVALID_ID, sourceid = INVALID_ID;
    UBYTE redbits	= 4
    	, greenbits	= 4
	, bluebits	= 4;
    ULONG dipf_musthave = 0
    	, dipf_mustnothave = 0;
    struct displayinfo_db *db;
    ULONG i;
    
    ULONG found_id = INVALID_ID;

/*    ULONG maxdepth = 0;
    ULONG maxwidth = 0, maxheight = 0;
    UBYTE maxrb = 0, maxgb = 0, maxbb = 0;
*/    
    /* First try to get viewport */
    vp			= (struct ViewPort *)GetTagData(BIDTAG_ViewPort, (IPTR)NULL	, tags);
    monitorid		= GetTagData(BIDTAG_MonitorID		, monitorid		, tags);
    sourceid		= GetTagData(BIDTAG_SourceID		, sourceid		, tags);
    depth		= GetTagData(BIDTAG_Depth		, depth			, tags);
    nominal_width	= GetTagData(BIDTAG_NominalWidth	, nominal_width		, tags);
    nominal_height	= GetTagData(BIDTAG_NominalHeight	, nominal_height	, tags);
    desired_width	= GetTagData(BIDTAG_DesiredWidth	, desired_width		, tags);
    desired_height	= GetTagData(BIDTAG_DesiredHeight	, desired_height	, tags);
    redbits		= GetTagData(BIDTAG_RedBits		, redbits		, tags);
    greenbits		= GetTagData(BIDTAG_GreenBits		, greenbits		, tags);
    bluebits		= GetTagData(BIDTAG_BlueBits		, bluebits		, tags);
    dipf_musthave	= GetTagData(BIDTAG_DIPFMustHave	, dipf_musthave		, tags);
    dipf_mustnothave	= GetTagData(BIDTAG_DIPFMustNotHave	, dipf_mustnothave	, tags);
    
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



#warning Implement Display mode attributes in the below function

VOID driver_FreeCModeList(struct List *modeList, struct GfxBase *GfxBase)
{
    struct CyberModeNode *node, *safe;
    
    ForeachNodeSafe(modeList, node, safe) {
	Remove((struct Node *)node);
	FreeMem(node, sizeof (struct CyberModeNode));
    }
    
    FreeMem(modeList, sizeof (struct List));
}

APTR driver_AllocCModeListTagList(struct TagItem *taglist, struct GfxBase *GfxBase )
{
    struct TagItem *tag, *tstate;
    
    ULONG minwidth = 320;
    ULONG maxwidth = 1600;
    ULONG minheight = 240;
    ULONG maxheight = 1200;
    ULONG mindepth = 8;
    ULONG maxdepth = 32;
    
    struct List *cybermlist = NULL;
    struct displayinfo_db *db;
    
    Object *gfxhidd;
    
    ULONG gmno;
    UWORD *cmodelarray = NULL;
    
    gfxhidd	= SDD(GfxBase)->gfxhidd;
    db		= SDD(GfxBase)->dispinfo_db;
    
    for (tstate = taglist; (tag = NextTagItem((const struct TagItem **)&tstate)); ) {
	switch (tag->ti_Tag) {
	    case CYBRMREQ_MinWidth:
	    	minwidth = (ULONG)tag->ti_Data;
		break;
		
	    case CYBRMREQ_MaxWidth:
	     	maxwidth = (ULONG)tag->ti_Data;
		break;
		
	    case CYBRMREQ_MinHeight:
	    	minheight = (ULONG)tag->ti_Data;
		break;
		
	    case CYBRMREQ_MaxHeight:
	    	maxheight = (ULONG)tag->ti_Data;
		break;
		
	    case CYBRMREQ_MinDepth:
	    	mindepth = (ULONG)tag->ti_Data;
		break;
		
	    case CYBRMREQ_MaxDepth:
	    	maxdepth = (ULONG)tag->ti_Data;
		break;
		
	    case CYBRMREQ_CModelArray:
	    	cmodelarray = (UWORD *)tag->ti_Data;
		break;
		
	    default:
	    	kprintf("!!! UNKNOWN TAG PASSED TO AllocCModeListTagList\n");
		break;
		
	
	} 	
    }
    
    /* Allocate the exec list */
    cybermlist = AllocMem(sizeof (struct List), MEMF_CLEAR);
    if (NULL == cybermlist)
    	return NULL;
    
    
    /* Go through the displayinfo db looking for cgfx modes */

ObtainSemaphoreShared(&db->sema);	
    NEWLIST(cybermlist);
	    
    for (gmno = 0; gmno < db->nummodes; gmno ++) {
	    
	struct CyberModeNode *cmnode;
	UWORD *cyberpixfmts;
	ULONG width, height;
	ULONG numpfs;
	Object *gm;
	    
	ULONG pfno;
	
	gm = db->dispitems[gmno].gfxmode;


	GetAttr(gm, aHidd_GfxMode_Width,  &width);
	GetAttr(gm, aHidd_GfxMode_Height, &height);
	
	if (	width < minwidth
	     || width > maxwidth
	     || height < minheight
	     || height > maxheight) {
	     
	     continue;
	}
	GetAttr(gm, aHidd_GfxMode_NumPixFmts, &numpfs);
	
	for (pfno = 0; pfno < numpfs; pfno ++) {
	    Object *pf;
	    ULONG depth;
	    
	    pf = HIDD_GM_LookupPixFmt(gm, pfno);
	
	    /* Get the pxifmt info */
	    GetAttr(pf, aHidd_PixFmt_Depth, &depth);
	    
	    if (depth < mindepth || depth > maxdepth)
	    	continue;
		
	    /* Check whether the gfxmode is the correct pixel format */
	    if (NULL != cmodelarray) {
	    
		HIDDT_StdPixFmt stdpf;
		UWORD cyberpf;
		BOOL found = FALSE;
		/* Get the gfxmode pixelf format */
		GetAttr(pf, aHidd_PixFmt_StdPixFmt, &stdpf);
		
		cyberpf = hidd2cyber_pixfmt(stdpf, GfxBase);
		if (cyberpf == (UWORD)-1)
		    continue;	/* Unknown format */
			
		for (cyberpixfmts = cmodelarray; *cyberpixfmts; cyberpixfmts ++) {
		    /* See if the stdpixfmt is present in the array */
		    if (*cyberpixfmts == cyberpf) {
			found = TRUE;
			    break;
		    }
			
		} /* for (each supplied pixelformat in the cmodelarray) */
		    
		if (!found)
		    continue; /* PixFmt not wanted, just continue with next node */
		
	     } /* if (cmodelarray supplied in the taglist) */
		
	    /* Allocate a cybergfx modeinfo struct */
	    
	    cmnode = AllocMem(sizeof (struct CyberModeNode), MEMF_CLEAR);
	    if (NULL == cmnode)
		goto failexit;
		
	    cmnode->Width	= width;
	    cmnode->Height	= height;
	    cmnode->Depth	= depth;
	    cmnode->DisplayTagList = NULL;
	    
	    snprintf( cmnode->ModeText
		, DISPLAYNAMELEN
		, "AROS: %dx%dx%d"
		, width, height, depth
	    );
		
	    /* Keep track of the node */
	    AddTail(cybermlist, (struct Node *)cmnode);
		
	
	} /* for (pixfmts in a gfxmode) */
	
    } /* for (gfxmodes in the dispinfo db) */

ReleaseSemaphore(&db->sema);    
    return cybermlist;
    
failexit:

ReleaseSemaphore(&db->sema);    

    if (NULL != cybermlist)
     	driver_FreeCModeList(cybermlist, GfxBase);
    
    return NULL;
}





ULONG driver_BestCModeIDTagList(struct TagItem *tags, struct GfxBase *GfxBase)
{
    struct TagItem *tag, *tstate;
      
    ULONG nominal_width, nominal_height, depth;
    ULONG monitorid;
    STRPTR boardname;
    ULONG modeid;
      
    nominal_width	= 800;
    nominal_height	= 600;
    depth		= 8;
    monitorid		= 0;
    boardname		= "Blah";
      
    for (tstate = tags; (tag = NextTagItem((const struct TagItem **)&tstate)); ) {
    	switch (tag->ti_Tag) {
	    case CYBRBIDTG_Depth:
	    	depth = tag->ti_Data;
	    	break;
	
	    case CYBRBIDTG_NominalWidth:
	        nominal_width = tag->ti_Data;
	    	break;

	    case CYBRBIDTG_NominalHeight:
	        nominal_height = tag->ti_Data;
	    	break;

	    case CYBRBIDTG_MonitorID:
	        monitorid = tag->ti_Data;
	    	break;

	    case CYBRBIDTG_BoardName:
	    	boardname = (STRPTR)tag->ti_Data;
	    	break;
		
	    default:
	    	kprintf("!!! UNKOWN ATTR PASSED TO BestCModeIDTagList(): %x !!!\n"
			, tag->ti_Tag);
		break;

	} /* switch () */
      
    } /* for (each tag in the taglist) */
    
    if (depth < 8 )  {
    
    	/* No request for a cgfx mode */
    	modeid = INVALID_ID;
    
    } else {
	/* Get the best modeid */
	struct TagItem modetags[] = {
	    { BIDTAG_NominalWidth,	nominal_width	},
	    { BIDTAG_NominalHeight,	nominal_height	},
	    { BIDTAG_Depth,		depth		},
	    { BIDTAG_MonitorID,		monitorid	},
	    { TAG_DONE, 0UL }
	};
	
	modeid = BestModeIDA(modetags);
    }
    
    /* Use the data to select a mode */
    return modeid;
      
}

ULONG driver_GetCyberIDAttr(ULONG attribute, ULONG id, struct GfxBase *GfxBase)
{
    /* First lookup the pixfmt for the ID */
    ULONG majoridx, minoridx;
    ULONG retval;
    ULONG depth;
    
    struct displayinfo_db *db;
    Object *gm, *pf;
    
    db = (struct displayinfo_db *)SDD(GfxBase)->dispinfo_db;
    
    majoridx = MAJORID2NUM(id);
    minoridx = MINORID2NUM(id);
    
    retval = (ULONG)-1;
    
ObtainSemaphoreShared(&db->sema);

    if (majoridx < db->nummodes)
    	goto exit;

    gm = db->dispitems[majoridx].gfxmode;
    pf = HIDD_GM_LookupPixFmt(gm, minoridx);
    if (NULL == pf)
    	goto exit;
    
    GetAttr(pf, aHidd_PixFmt_Depth, &depth);
    
    if (depth < 8) {
    	kprintf("!!! TRYING TO GET ATTR FROM NON-CGFX MODE IN GetCyberIDAttr() !!!\n");
	retval = (ULONG)-1;
    } else {
    
	switch (attribute) {
	    case CYBRIDATTR_PIXFMT: {
	    	HIDDT_StdPixFmt stdpf;
		
		GetAttr(pf, aHidd_PixFmt_StdPixFmt, &stdpf);
		
		retval = hidd2cyber_pixfmt(stdpf, GfxBase);
		if (-1 == retval) {
		    kprintf("!!! NO CGFX PIXFMT IN GetCyberIDAttr() !!!\n");
		}
		
	    	break; }
	
	    case CYBRIDATTR_DEPTH:
	     	retval = depth;
		break;
		
	
	    case CYBRIDATTR_WIDTH:
	    	GetAttr(gm, aHidd_GfxMode_Width, &retval);
		break;
		
	    case CYBRIDATTR_HEIGHT:
	    	GetAttr(gm, aHidd_GfxMode_Height, &retval);
		break;
		
	    case CYBRIDATTR_BPPIX:
	    	GetAttr(pf, aHidd_PixFmt_BytesPerPixel, &retval);
		break;
		
	    default:
	    	kprintf("!!! UNKONOW ATTRIBUTE IN GetCyberIDAttr(): %x !!!\n"
			, attribute);
		retval = (ULONG)-1;
		break;
	    	
    	
	}
    }
exit:    
ReleaseSemaphore(&db->sema);    
    return retval;
}


BOOL driver_IsCyberModeID(ULONG modeid, struct GfxBase *GfxBase)
{
    struct displayinfo_db *db;
    ULONG majoridx, minoridx;
    BOOL iscyber = FALSE;
    
    db = (struct displayinfo_db *)SDD(GfxBase)->dispinfo_db;
    
    /* Lookup the gfxmode and pixfmt */
    majoridx = MAJORID2NUM(modeid);
    minoridx = MINORID2NUM(modeid);
    
ObtainSemaphoreShared(&db->sema);    
    if (majoridx < db->nummodes) {
    	Object *pf;
	
	pf = HIDD_GM_LookupPixFmt(db->dispitems[majoridx].gfxmode, minoridx);
	if (NULL != pf) {
	    HIDDT_StdPixFmt stdpf;
	    
	    GetAttr(pf, aHidd_PixFmt_StdPixFmt, &stdpf);
	    if (((ULONG)-1) != hidd2cyber_pixfmt(stdpf, GfxBase)) {
	    	iscyber = TRUE;
	    }
	    
	}
    }
    
ReleaseSemaphore(&db->sema);
    return iscyber;
    
}
