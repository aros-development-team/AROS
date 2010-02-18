/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
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
