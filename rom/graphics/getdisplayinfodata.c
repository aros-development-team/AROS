/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function GetDisplayInfoData()
    Lang: english
*/
#include <aros/debug.h>
#include <proto/graphics.h>
#include <graphics/displayinfo.h>
#include <hidd/graphics.h>
#include "dispinfo.h"
#include <proto/oop.h>
#include <stdio.h>
#include <string.h>
#include "graphics_intern.h"

struct size_check
{
    ULONG struct_id;
    ULONG struct_size;
    STRPTR struct_name;
};

extern const struct size_check size_checks[];

static BOOL check_sizes(ULONG tagID, ULONG size);
static ULONG compute_numbits(HIDDT_Pixel mask);

#define DLONGSZ     	    (sizeof (ULONG) * 2)
#define DTAG_TO_IDX(dtag)   (((dtag) & 0x0000F000) >> 12)

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH5(ULONG, GetDisplayInfoData,

/*  SYNOPSIS */
        AROS_LHA(DisplayInfoHandle, handle, A0),
        AROS_LHA(UBYTE *, buf, A1),
        AROS_LHA(ULONG, size, D0),
        AROS_LHA(ULONG, tagID, D1),
        AROS_LHA(ULONG, ID, D2),

/*  LOCATION */
        struct GfxBase *, GfxBase, 126, Graphics)

/*  FUNCTION

    INPUTS
        handle - displayinfo handle
        buf    - pointer to destination buffer
        size   - buffer size in bytes
        tagID  - data chunk type
        ID     - displayinfo identifier, optionally used if handle is NULL

    RESULT
        result - if positive, number of bytes actually transferred
                 if zero, no information for ID was available

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        FindDisplayInfo() NextDisplayInfo() graphics/displayinfo.h

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)
    
    struct QueryHeader  *qh;
    ULONG   	    	structsize;
    OOP_Object      	*sync, *pf;
    HIDDT_ModeID    	hiddmode;
    ULONG   	    	modeid;

    if (NULL == handle)
    {
	if ((ULONG)INVALID_ID != ID)
	{
	    /* Check that ID is a valid modeid */
	    handle = FindDisplayInfo(ID);
	} 
	else
	{
	    D(bug("!!! INVALID MODE ID IN GetDisplayInfoData()\n"));
	    return 0;
	}
    }
    
    if (NULL == handle)
    {
	D(bug("!!! COULD NOT GET HANDLE IN GetDisplayInfoData()\n"));
	return 0;
    }
    
    modeid = (ULONG)handle;
    hiddmode = (HIDDT_ModeID)AMIGA_TO_HIDD_MODEID(modeid);
    
    /* Get mode info from the HIDD */
    if (!HIDD_Gfx_GetMode(SDD(GfxBase)->gfxhidd, hiddmode, &sync, &pf))
    {
	D(bug("NO VALID MODE PASSED TO GetDisplayInfoData() !!!\n"));
	return 0;
    }
    
    
    D(bug("GetDisplayInfoData(handle=%d, modeid=%x, tagID=%x)\n"
    	, (ULONG)handle, modeid, tagID));
	
    
    /* Build the queryheader */
    if (!check_sizes(tagID, size)) return 0;
    
    memset(buf, 0, size);
    
    /* Fill in the queryheader */
    qh = (struct QueryHeader *)buf;
    qh->StructID  = tagID;
    qh->DisplayID = modeid;
    qh->SkipID	  = TAG_SKIP;
    
    structsize = size_checks[DTAG_TO_IDX(tagID)].struct_size;
    qh->Length	  = (structsize + (DLONGSZ - 1)) / DLONGSZ;
    
    switch (tagID)
    {
    	case DTAG_DISP:
	{
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
	    
	    di->Resolution.x = 22;
	    di->Resolution.y = 22;
	    
/*	    
	    di->Resolution.x = ?;
	    di->Resolution.y = ?;
	    di->PixelSpeed = ?;
	    di->NumStdSprites = ?
	    di->SpriteResolution.x = ?;
	    di->SpriteResolution.y = ?;
*/	    
	    
	    break;
	}
	    
	case DTAG_DIMS:
	{
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
	    break;
	}
	    
	case DTAG_MNTR:
	{
	    struct MonitorInfo *mi;
	    struct MonitorSpec *mspc;
	    struct displayinfo_db *db;
	    ULONG majoridx;
	    
	    db = (struct displayinfo_db *)SDD(GfxBase)->dispinfo_db;
    	    
	    ObtainSemaphoreShared(&db->sema);
	    majoridx = MAJORID2NUM(modeid);
	    if (majoridx >= db->num_mspecs)
	    {
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
	    break;
	}
	    
	case DTAG_NAME:
	{
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
	    break;
	}
	    
	default:
	    D(bug("!!! UNKNOWN tagID IN CALL TO GetDisplayInfoData() !!!\n"));
	    break;
    	
    }
    
    D(bug("GDID: %d\n", structsize));

    return structsize;

    AROS_LIBFUNC_EXIT

} /* GetDisplayInfoData */

/****************************************************************************************/

#define PRIV_DTAG_QHDR 0x80005000

static const struct size_check size_checks[] = 
{
    { DTAG_DISP,	sizeof(struct DisplayInfo),	"DisplayInfo"	},
    { DTAG_DIMS,	sizeof(struct DimensionInfo),	"DimensionInfo"	},
    { DTAG_MNTR,	sizeof(struct MonitorInfo),	"MonitorInfo"	},
    { DTAG_NAME,	sizeof(struct NameInfo), 	"NameInfo"	},
    { DTAG_VEC,		sizeof(struct VecInfo), 	"VecInfo"	},
    { PRIV_DTAG_QHDR,	sizeof(struct QueryHeader), 	"QueryHeader"	}
    
};

/****************************************************************************************/

static BOOL check_sizes(ULONG tagID, ULONG size)
{
    ULONG idx;
    const struct size_check *sc;
    
    idx = DTAG_TO_IDX(tagID);
    
    if (idx > 5)
    {
    	D(bug("!!! INVALID tagID TO GetDisplayInfoData"));
	return FALSE;
    }
    
    sc = &size_checks[idx];
    if (sc->struct_id != tagID)
    {
    	D(bug("!!! INVALID tagID TO GetDisplayInfoData"));
	return FALSE;
    }
    
    if (sc->struct_size > size)
    {
    	D(bug("!!! NO SPACE FOR %s IN BUFFER SUPPLIED TO GetDisplayInfoData !!!\n"
		, sc->struct_name));
	return FALSE;
    }
    
    return TRUE;
}

/****************************************************************************************/

static ULONG compute_numbits(HIDDT_Pixel mask)
{
    ULONG i;
    ULONG numbits = 0;
    
    for (i = 0; i <= 31; i ++)
    {
    	if (mask & (1L << i)) numbits ++;
    }
    
    return numbits;
}

/****************************************************************************************/


/****************************************************************************************/
