/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

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
#include "gfxfuncsupport.h"
#include "dispinfo.h"

#define DEBUG 0
#include <aros/debug.h>

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
    IPTR numsyncs;
    
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
    const struct TagItem *tstate;
    struct TagItem *tag;
    
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
    
    for (tstate = taglist; (tag = NextTagItem(&tstate)); ) {
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
	IPTR width, height, depth;
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
    const struct TagItem *tstate;
    struct TagItem *tag;
      
    ULONG nominal_width, nominal_height, depth;
    ULONG monitorid;
    STRPTR boardname;
    ULONG modeid;
      
    nominal_width	= 800;
    nominal_height	= 600;
    depth		= 8;
    monitorid		= 0;
    boardname		= "Blah";
      
    for (tstate = tags; (tag = NextTagItem(&tstate)); ) {
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
	    { BIDTAG_DesiredWidth,	nominal_width	},
	    { BIDTAG_DesiredHeight,	nominal_height	},
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
    IPTR retval;
    OOP_Object *sync, *pf;
    HIDDT_ModeID hiddmode;
    
    hiddmode = AMIGA_TO_HIDD_MODEID(id);
    
    retval = (ULONG)-1;
    
    if (HIDD_Gfx_GetMode(SDD(GfxBase)->gfxhidd, hiddmode, &sync, &pf)) {
        IPTR depth;
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
    HIDDT_ModeID hiddmode = 0;
    OOP_Object *sync, *pf;
    
    hiddmode = AMIGA_TO_HIDD_MODEID(modeid);
    
    if (HIDD_Gfx_GetMode(SDD(GfxBase)->gfxhidd, hiddmode, &sync, &pf)) {
	HIDDT_StdPixFmt stdpf;
	    
	OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, &stdpf);
	if (((UWORD)-1) != hidd2cyber_pixfmt(stdpf, GfxBase)) {
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
                    best_resolution = res;
		}
		
	    	best_depth = depth;
	    }
	    
	}
	
	HIDD_Gfx_ReleaseModeIDs(gfxhidd, modes);
    }
    
    return ret;
    
}
