/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

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

#define DEBUG 0
#include <aros/debug.h>

/* stegerg: check */

/* #define NOTNULLMASK 0x10000000 --> trouble with more than 4 gfxmodes: 4 << 26 = 0x10000000 */
#define NOTNULLMASK 0x0001000

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

/*
    ModeID construction is really private to the HIDD so
    this is a hack
*/

#define AMIGA_TO_HIDD_MODEID(modeid)		\
    ( ((modeid) == INVALID_ID) 			\
		? vHidd_ModeID_Invalid  	\
		: ( (MAJORID2NUM(modeid) << 16) | MINORID2NUM(modeid)) )

#define HIDD_TO_AMIGA_MODEID(modeid)			\
    ( ((modeid) == vHidd_ModeID_Invalid) 	\
		? INVALID_ID			\
		: (GENERATE_MODEID((modeid) >> 16, (modeid) & 0x0000FFFF)) )


struct displayinfo_db {
    struct MonitorSpec *mspecs;
    ULONG num_mspecs;
    struct SignalSemaphore sema;
};

HIDDT_ModeID get_hiddmode_for_amigamodeid(ULONG modeid, struct GfxBase *GfxBase)
{
    return AMIGA_TO_HIDD_MODEID(modeid);
}

VOID destroy_dispinfo_db(APTR dispinfo_db, struct GfxBase *GfxBase)
{
    struct displayinfo_db *db;
    
    db = (struct displayinfo_db *)dispinfo_db;
    
    ObtainSemaphore(&db->sema);
    
    if (NULL != db->mspecs) {
	FreeMem(db->mspecs, sizeof (struct MonitorSpec) * db->num_mspecs);
	db->mspecs = NULL;
	db->num_mspecs = 0;
    }
    
    ReleaseSemaphore(&db->sema);
    
    FreeMem(db, sizeof (*db));
    
}

APTR build_dispinfo_db(struct GfxBase *GfxBase)
{
    struct displayinfo_db *db;
    ULONG numsyncs;
    
    db = AllocMem(sizeof (struct displayinfo_db), MEMF_PUBLIC | MEMF_CLEAR);
    if (NULL != db) {
	
    	InitSemaphore(&db->sema);
    
    	/* Get the number of possible modes in the gfxhidd */
	OOP_GetAttr(SDD(GfxBase)->gfxhidd, aHidd_Gfx_NumSyncs, &numsyncs);
	
	db->num_mspecs = numsyncs;
	
    	/* Allocate a table to hold all the monitorspecs  */
    	db->mspecs = AllocMem(sizeof (struct MonitorSpec) * db->num_mspecs, MEMF_PUBLIC | MEMF_CLEAR);
    	if (NULL != db->mspecs) {
	    return (APTR)db;
	}
	destroy_dispinfo_db(db, GfxBase);
    }
    return NULL;
}


ULONG driver_NextDisplayInfo(ULONG lastid, struct GfxBase *GfxBase)
{
    OOP_Object *sync, *pixfmt;
    
    HIDDT_ModeID hiddmode;
    ULONG id;
    
    hiddmode = AMIGA_TO_HIDD_MODEID(lastid);
    
    /* Get the next modeid */
    hiddmode = HIDD_Gfx_NextModeID(SDD(GfxBase)->gfxhidd, hiddmode, &sync, &pixfmt);
    
    id = HIDD_TO_AMIGA_MODEID(hiddmode);
    
    return id;
    
}


DisplayInfoHandle driver_FindDisplayInfo(ULONG id, struct GfxBase *GfxBase)
{
    DisplayInfoHandle ret = NULL;
    HIDDT_ModeID hiddmode;
    OOP_Object *sync, *pixfmt;
    
    D(bug("FindDisplayInfo(id=%x)\n", id));
    
    /* Check for the NOTNULLMASK */
    if ((id & NOTNULLMASK) != NOTNULLMASK) {
    	D(bug("!!! NO AROS MODEID IN FindDisplayInfo() !!!\n"));
    	return NULL;
    }
    
    hiddmode = AMIGA_TO_HIDD_MODEID(id);
    
    /* Try to get mode info for the mode */
    if (!HIDD_Gfx_GetMode(SDD(GfxBase)->gfxhidd, hiddmode, &sync, &pixfmt)) {
	D(bug("!!! NO AROS MODEID IN FindDisplayInfo() !!!\n"));
	return NULL;
    }
    
    ret = (DisplayInfoHandle)id;

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
    	D(bug("!!! INVALID TAGID TO GetDisplayInfoData"));
	return FALSE;
    }
    
    sc = &size_checks[idx];
    if (sc->struct_id != tagid) {
    	D(bug("!!! INVALID TAGID TO GetDisplayInfoData"));
	return FALSE;
    }
    
    if (sc->struct_size > size) {
    	D(bug("!!! NO SPACE FOR %s IN BUFFER SUPPLIED TO GetDisplayInfoData !!!\n"
		, sc->struct_name));
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
    struct QueryHeader *qh;
    ULONG structsize;
    OOP_Object *sync, *pf;
    HIDDT_ModeID hiddmode;
    ULONG modeid;

    if (NULL == handle) {
	if (INVALID_ID != id2) {
	    /* Check that id2 is a valid modeid */
	    handle = FindDisplayInfo(id2);
	} else {
	    D(bug("!!! INVALID MODE ID IN GetDisplayInfoData()\n"));
	    return 0;
	}
    }
    
    if (NULL == handle) {
	D(bug("!!! COULD NOT GET HANDLE IN GetDisplayInfoData()\n"));
	return 0;
    }
    
    modeid = (ULONG)handle;
    hiddmode = AMIGA_TO_HIDD_MODEID(modeid);
    
    /* Get mode info from the HIDD */
    if (!HIDD_Gfx_GetMode(SDD(GfxBase)->gfxhidd, hiddmode, &sync, &pf)) {
	D(bug("NO VALID MODE PASSED TO GetDisplayInfoData() !!!\n"));
	return 0;
    }
    
    
    D(bug("GetDisplayInfoData(handle=%d, modeid=%x, tagid=%x)\n"
    	, (ULONG)handle, modeid, tagid));
	
    
    /* Build the queryheader */
    if (!check_sizes(tagid, size)) return 0;
    
    memset(buf, 0, size);
    
    /* Fill in the queryheader */
    qh = (struct QueryHeader *)buf;
    qh->StructID  = tagid;
    qh->DisplayID = modeid;
    qh->SkipID	  = TAG_SKIP;
    
    structsize = size_checks[DTAG_TO_IDX(tagid)].struct_size;
    qh->Length	  = (structsize + (DLONGSZ - 1)) / DLONGSZ;
    
    switch (tagid) {
    	case DTAG_DISP: {
	    struct DisplayInfo *di;
	    HIDDT_Pixel redmask, greenmask, bluemask;
	    
	    di = (struct DisplayInfo *)buf;
	    
	    /* All modes returned from the HIDD are available */
	    di->NotAvailable = FALSE;
	    
	    /* Set the propertyflags */
	    di->PropertyFlags = DIPF_IS_FOREIGN | DIPF_IS_WB;
	    
	    /* We simulate AGA. This field is really obsolete */
	    di->PaletteRange = 4096;

	    /* Compute red green and blue bits */
	    OOP_GetAttr(pf, aHidd_PixFmt_RedMask,	&redmask);
	    OOP_GetAttr(pf, aHidd_PixFmt_GreenMask, &greenmask);
	    OOP_GetAttr(pf, aHidd_PixFmt_BlueMask,	&bluemask);
	    
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
	    
	    OOP_GetAttr(pf,   aHidd_PixFmt_Depth, &depth);
	    OOP_GetAttr(sync, aHidd_Sync_HDisp,   &width);
	    OOP_GetAttr(sync, aHidd_Sync_VDisp,   &height);
	    
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
	    struct displayinfo_db *db;
	    ULONG majoridx;
	    
	    db = (struct displayinfo_db *)SDD(GfxBase)->dispinfo_db;
ObtainSemaphoreShared(&db->sema);
	    majoridx = MAJORID2NUM(modeid);
	    if (majoridx >= db->num_mspecs) {
		D(bug("!!! INVALID MODE ID IN GetDisplayInfoData(DTAG_MNTR) !!!\n"));
ReleaseSemaphore(&db->sema);
		return 0;
	    }
	    
	    
	    mi = (struct MonitorInfo *)buf;
	    
	    mspc = &db->mspecs[majoridx];
	    
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
	    mi->PreferredModeID = modeid;
	    mi->Compatibility = MCOMPAT_NOBODY;
	    
	    /* Fill info into the monitorspec. It is by default set to all 0s */
	    mspc->ms_Node.xln_Pred = mspc->ms_Node.xln_Succ = NULL;
	    mspc->ms_Node.xln_Type = MONITOR_SPEC_TYPE;
	    mspc->ms_Node.xln_Name = "AROS.monitor";
	    mspc->total_rows = mi->TotalRows;
	    mspc->total_colorclocks = mi->TotalColorClocks;
	    mspc->min_row = mi->MinRow;
	    
	    /* What to put in here ? */
	    mspc->ms_Flags = 0;
ReleaseSemaphore(&db->sema);
	    break; }
	    
	case DTAG_NAME: {
	    struct NameInfo *ni;
	    ULONG depth, width, height;
	    
	    OOP_GetAttr(pf,   aHidd_PixFmt_Depth,	&depth);
	    OOP_GetAttr(sync, aHidd_Sync_HDisp, 	&width);
	    OOP_GetAttr(sync, aHidd_Sync_VDisp,	&height);
	    
	    ni = (struct NameInfo *)buf;
	    
	    snprintf(ni->Name, DISPLAYNAMELEN
	    	, "AROS: %ldx%ldx%ld"
		, width, height, depth
	    );
	    break; }
	    
	default:
	    D(bug("!!! UNKNOWN TAGID IN CALL TO GetDisplayInfoData() !!!\n"));
	    break;
    	
    }
    

D(bug("GDID: %d\n", structsize));
    return structsize;
}


ULONG driver_GetVPModeID(struct ViewPort *vp, struct GfxBase *GfxBase)
{
    ULONG modeid;
    D(bug(" GetVPModeID returning %x\n", vp->ColorMap->VPModeID));
    modeid = vp->ColorMap->VPModeID;
    
    D(bug("RETURNING\n"));
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
    
    ULONG found_id = INVALID_ID;
    HIDDT_ModeID hiddmode;

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
	    D(bug("!!! Passing viewport with NULL vp->RasInfo->BitMap to BestModeIDA() !!!\n"));
	}
    }
    
    if (INVALID_ID != sourceid) {
#warning Fix this

/* I do not understand what the docs state about this */
	
    }
    
    /* OK, now we try to search for a mode that has the supplied charateristics */
    
    hiddmode = vHidd_ModeID_Invalid;
    for (;;) {
	OOP_Object *sync, *pf;
	
	ULONG redmask, greenmask, bluemask;
	ULONG gm_depth, gm_width, gm_height;
	hiddmode = HIDD_Gfx_NextModeID(SDD(GfxBase)->gfxhidd, hiddmode, &sync, &pf);
	if (vHidd_ModeID_Invalid == hiddmode)
	    break;

	OOP_GetAttr(pf, aHidd_PixFmt_RedMask,	&redmask);
	OOP_GetAttr(pf, aHidd_PixFmt_GreenMask,	&greenmask);
	OOP_GetAttr(pf, aHidd_PixFmt_BlueMask,	&bluemask);
	
	OOP_GetAttr(pf, aHidd_PixFmt_Depth,		&gm_depth);
	OOP_GetAttr(sync, aHidd_Sync_HDisp,		&gm_width);
	OOP_GetAttr(sync, aHidd_Sync_VDisp,		&gm_height);
	
	if ( /*   compute_numbits(redmask)   >= redbits
	     && compute_numbits(greenmask) >= greenbits
	     && compute_numbits(bluemask)  >= bluebits
	    
	     && */gm_depth  >= depth
	     && gm_width  >= desired_width
	     && gm_height >= desired_height) {
#warning Fix this 	    
	    	/* We return the first modeid that fulfill the criterias.
	 	      Instead we should find the mode that has:
		           - largest possible depth.
			   - width/height that are nearest above or equal the desired ones
	    	*/
	    found_id = HIDD_TO_AMIGA_MODEID(hiddmode);
	    break;
	     
	} 
    } /* for (each HIDD modeid) */
    
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
    
    OOP_Object *gfxhidd;
    
    UWORD *cmodelarray = NULL;
    HIDDT_ModeID *hiddmodes = NULL, *hmptr;
    struct TagItem querytags[] = { { TAG_DONE, 0UL } };
    
    gfxhidd	= SDD(GfxBase)->gfxhidd;
    
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
	    	D(bug("!!! UNKNOWN TAG PASSED TO AllocCModeListTagList\n"));
		break;
	} 	
    }
    
    /* Allocate the exec list */
    cybermlist = AllocMem(sizeof (struct List), MEMF_CLEAR);
    if (NULL == cybermlist)
    	return NULL;
    
    
    NEWLIST(cybermlist);
    
    /* Get all HIDD modes */
    hiddmodes = HIDD_Gfx_QueryModeIDs(gfxhidd, querytags);
    if (NULL == hiddmodes)
	goto failexit;
	    
    
    for (hmptr = hiddmodes; *hmptr != vHidd_ModeID_Invalid; hmptr ++) {
	    
	struct CyberModeNode *cmnode;
	UWORD *cyberpixfmts;
	ULONG width, height, depth;
	OOP_Object *sync, *pf;
	
	if (!HIDD_Gfx_GetMode(gfxhidd, *hmptr, &sync, &pf)) {
	    /* This should never happen because HIDD_GfxWueryModeIDs() should
	       only return valid modes
	    */
	    D(bug("!!! UNABLE TO GET HIDD MODE INFO IN AllocCModeListTagList() !!!\n"));
	    D(bug("!!! THIS SHOULD *NEVER* HAPPEN !!!\n"));
	    goto failexit;
	}

	OOP_GetAttr(sync, aHidd_Sync_HDisp,  &width);
	OOP_GetAttr(sync, aHidd_Sync_VDisp, &height);
	
	if (	width < minwidth
	     || width > maxwidth
	     || height < minheight
	     || height > maxheight) {
	     
	     continue;
	}
	    
	/* Get the pxifmt info */
	OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
	    
	if (depth < mindepth || depth > maxdepth)
	    continue;
		
	/* Check whether the gfxmode is the correct pixel format */
	if (NULL != cmodelarray) {
	    HIDDT_StdPixFmt stdpf;
	    UWORD cyberpf;
	    BOOL found = FALSE;
	    
	    /* Get the gfxmode pixelf format */
	    OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, &stdpf);
		
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
		, "AROS: %ldx%ldx%ld"
		, width, height, depth
	);
		
	/* Keep track of the node */
	AddTail(cybermlist, (struct Node *)cmnode);
	
    } /* for (modeids returned from the HIDD) */

    return cybermlist;
    
failexit:

    if (NULL != hiddmodes)
	HIDD_Gfx_ReleaseModeIDs(gfxhidd, hiddmodes);

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
	    	D(bug("!!! UNKOWN ATTR PASSED TO BestCModeIDTagList(): %x !!!\n", tag->ti_Tag));
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
    ULONG retval;
    OOP_Object *sync, *pf;
    HIDDT_ModeID hiddmode;
    
    hiddmode = AMIGA_TO_HIDD_MODEID(id);
    
    retval = (ULONG)-1;
    
    if (HIDD_Gfx_GetMode(SDD(GfxBase)->gfxhidd, hiddmode, &sync, &pf)) {
        ULONG depth;
	OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
    
	if (depth < 8) {
    	    D(bug("!!! TRYING TO GET ATTR FROM NON-CGFX MODE IN GetCyberIDAttr() !!!\n"));
	    retval = (ULONG)-1;
	} else {
    
	    switch (attribute) {
		case CYBRIDATTR_PIXFMT: {
	    	    HIDDT_StdPixFmt stdpf;
		
		    OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, &stdpf);
		
		    retval = hidd2cyber_pixfmt(stdpf, GfxBase);
		    if (-1 == retval) {
			D(bug("!!! NO CGFX PIXFMT IN GetCyberIDAttr() !!!\n"));
		    }
	    	    break; }
	
		case CYBRIDATTR_DEPTH:
	     	    retval = depth;
		    break;
	
		case CYBRIDATTR_WIDTH:
	    	    OOP_GetAttr(sync, aHidd_Sync_HDisp, &retval);
		    break;
		
		case CYBRIDATTR_HEIGHT:
	    	    OOP_GetAttr(sync, aHidd_Sync_VDisp, &retval);
		    break;
		
		case CYBRIDATTR_BPPIX:
	    	    OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, &retval);
		    break;
		
		default:
	    	    D(bug("!!! UNKONOW ATTRIBUTE IN GetCyberIDAttr(): %x !!!\n"
			, attribute));
		    retval = (ULONG)-1;
		    break;
	    	
    	
	    }
	}
    }
    return retval;
}


BOOL driver_IsCyberModeID(ULONG modeid, struct GfxBase *GfxBase)
{
    BOOL iscyber = FALSE;
    HIDDT_ModeID hiddmode;
    OOP_Object *sync, *pf;
    
    hiddmode = AMIGA_TO_HIDD_MODEID(hiddmode);
    
    if (HIDD_Gfx_GetMode(SDD(GfxBase)->gfxhidd, hiddmode, &sync, &pf)) {
	HIDDT_StdPixFmt stdpf;
	    
	OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, &stdpf);
	if (((ULONG)-1) != hidd2cyber_pixfmt(stdpf, GfxBase)) {
	    	iscyber = TRUE;
	}
    }
    return iscyber;
}


HIDDT_ModeID get_best_resolution_and_depth(struct GfxBase *GfxBase)
{
    HIDDT_ModeID ret = vHidd_ModeID_Invalid;
    OOP_Object *gfxhidd;
    HIDDT_ModeID *modes, *m;
    struct TagItem querytags[] = { { TAG_DONE, 0UL } };
    
    gfxhidd = SDD(GfxBase)->gfxhidd;
    
    /* Query the gfxhidd for all modes */
    modes = HIDD_Gfx_QueryModeIDs(gfxhidd, querytags);
    if (NULL != modes) {
	ULONG best_resolution = 0;
	ULONG best_depth = 0;
	
	for (m = modes; vHidd_ModeID_Invalid != *m; m ++) {
    	    OOP_Object *sync, *pf;
	    IPTR depth;
	    HIDD_Gfx_GetMode(gfxhidd, *m, &sync, &pf);
	    
	    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
	    if (depth >= best_depth) {
	    	IPTR width, height;
		ULONG res;
		
		OOP_GetAttr(sync, aHidd_Sync_HDisp, &width);
		OOP_GetAttr(sync, aHidd_Sync_VDisp, &height);
		
		res = width * height;
		if (res > best_resolution) {
		    ret = *m;
		}
		
	    	best_depth = depth;
	    }
	    
	}
	
	HIDD_Gfx_ReleaseModeIDs(gfxhidd, modes);
    }
    
    return ret;
    
}
