/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/arossupport.h>
#include <proto/utility.h>
#include <proto/graphics.h>

#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/rawfmt.h>

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

HIDDT_ModeID get_best_resolution_and_depth(OOP_Object *gfxhidd, struct GfxBase *GfxBase)
{
    HIDDT_ModeID ret = vHidd_ModeID_Invalid;
    HIDDT_ModeID *modes, *m;
    struct TagItem querytags[] = { { TAG_DONE, 0UL } };
    
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

BOOL CreateMonitorSpecs(ULONG card, struct monitor_driverdata *mdd, struct GfxBase *GfxBase)
{
    IPTR nsyncs = 0;
    ULONG i;
    
    OOP_GetAttr(mdd->gfxhidd, aHidd_Gfx_NumSyncs, &nsyncs);
    D(bug("[CreateMonitorSpecs] Card %u has %u syncs\n", card, nsyncs));
    
    mdd->specs = AllocMem(nsyncs * sizeof(struct MonitorSpec *), MEMF_ANY|MEMF_CLEAR);
    if (!mdd->specs)
        return FALSE;

    for (i = 0; i < nsyncs; i++) {
        OOP_Object *sync;
	struct MonitorSpec *mspc;
	STRPTR syncname = NULL;
	ULONG l;
	IPTR pixelClock = 0;
	IPTR total      = 0;
	IPTR hsstart = 0;
	IPTR hsstop  = 0;
	IPTR vsstart = 0;
	IPTR vsstop  = 0;

	DB2(bug("[CreateMonitorSpecs] Getting sync %u\n", i));
	sync = HIDD_Gfx_GetSync(mdd->gfxhidd, i);
	DB2(bug("[CreateMonitorSpecs] Sync object 0x%p\n", sync));

	mspc = GfxNew(MONITOR_SPEC_TYPE);
	if (!mspc)
	    return FALSE;

	OOP_GetAttr(sync, aHidd_Sync_Description, (IPTR *)&syncname);
	D(bug("[CreateMonitorSpecs] Adding spec %u (%s)\n", i, syncname));
	l = strlen(syncname) + 12; /* I hope 999 cards is enough :) */

	mspc->ms_Node.xln_Name = AllocMem(l, MEMF_ANY);
	if (!mspc->ms_Node.xln_Name) {
	    GfxFree(&mspc->ms_Node);
	    return FALSE;
	}
	NewRawDoFmt("display%u:%s", (VOID_FUNC)RAWFMTFUNC_STRING, mspc->ms_Node.xln_Name, card, syncname);

	OOP_GetAttr(sync, aHidd_Sync_HTotal, &total);
	mspc->total_rows = total;

	OOP_GetAttr(sync, aHidd_Sync_PixelClock, &pixelClock);
	OOP_GetAttr(sync, aHidd_Sync_HTotal, &total);
	/* Some poor man's drivers (like SDL hosted) can't provide any sync signal values
	   just because host system doesn't provide them. However Amiga software expects to
	   have something valid here. vTotal is dealt with by the driver (as a failback it's
	   equal to VDisp). There's no reasonable substitution for PixelClock, so we handle
	   it here. This value has nothing to do with real refresh rate, but we can do nothing
	   with it */
	if (pixelClock && total)
	    mspc->total_colorclocks = 100000000 / (pixelClock / total * 28);
	else
	    mspc->total_colorclocks = VGA_COLORCLOCKS;

	OOP_GetAttr(sync, aHidd_Sync_HSyncStart, &hsstart);
	OOP_GetAttr(sync, aHidd_Sync_HSyncEnd,   &hsstop );
	OOP_GetAttr(sync, aHidd_Sync_VSyncStart, &vsstart);
	OOP_GetAttr(sync, aHidd_Sync_VSyncEnd,   &vsstop );

	if (hsstart || hsstop || vsstart || vsstop) {
	    mspc->ms_Special = GfxNew(SPECIAL_MONITOR_TYPE);
	    if (mspc->ms_Special) {
	        mspc->ms_Flags |= MSF_REQUEST_SPECIAL;

	        mspc->ms_Special->hsync.asi_Start = hsstart;
		mspc->ms_Special->hsync.asi_Stop  = hsstop;
		mspc->ms_Special->vsync.asi_Start = vsstart;
		mspc->ms_Special->vsync.asi_Stop  = vsstop;

		mspc->ms_Special->reserved1 = sync;

		/* TODO: check if sync can be modified and install do_monitor() callback */

	    }
	}

	NewList(&mspc->DisplayInfoDataBase);
	InitSemaphore(&mspc->DisplayInfoDataBaseSemaphore);

	mdd->specs[i] = mspc;
	AddTail(&GfxBase->MonitorList, (struct Node *)mspc);
    }
    mdd->numspecs = nsyncs;
}

/* Just stubs, will do more things in future */

struct monitor_driverdata *FindDriver(ULONG modeid, struct GfxBase *GfxBase)
{
    return SDD(GfxBase);
}

struct MonitorSpec *FindMonitor(ULONG modeid, struct GfxBase *GfxBase)
{
    struct monitor_driverdata *mdd = FindDriver(modeid, GfxBase);
    ULONG idx = SYNC_IDX(modeid);
    
    if (idx < mdd->numspecs)
        return mdd->specs[idx];

    return NULL;
}

ULONG GetModeID(DisplayInfoHandle handle)
{
    return (ULONG)handle;
}
