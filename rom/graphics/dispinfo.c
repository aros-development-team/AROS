/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

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

#include <stddef.h>
#include <string.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"
#include "dispinfo.h"

HIDDT_ModeID get_best_resolution_and_depth(struct monitor_driverdata *mdd, struct GfxBase *GfxBase)
{
    HIDDT_ModeID ret = vHidd_ModeID_Invalid;
    struct DisplayInfoHandle *dh;
    ULONG best_resolution = 0;
    ULONG best_depth = 0;

    for (dh = mdd->modes; dh->id != vHidd_ModeID_Invalid; dh++) {
	OOP_Object *sync, *pf;
	IPTR depth;

	HIDD_Gfx_GetMode(mdd->gfxhidd, dh->id, &sync, &pf);
	OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);

	if (depth >= best_depth) {
	    IPTR width, height;
	    ULONG res;

	    OOP_GetAttr(sync, aHidd_Sync_HDisp, &width);
	    OOP_GetAttr(sync, aHidd_Sync_VDisp, &height);

	    res = width * height;
	    if (res > best_resolution) {
		ret = dh->id;
                best_resolution = res;
	    }
	    best_depth = depth;
	}
    }
    return ret;
}

void BestModeIDForMonitor(struct monitor_driverdata *mdd, ULONG dipf_musthave, ULONG dipf_mustnothave,
			  UBYTE redbits, UBYTE greenbits, UBYTE bluebits, UBYTE depth, STRPTR boardname,
			  UWORD nominal_width, UWORD nominal_height, UWORD desired_width, UWORD desired_height,
			  ULONG *found_id, UWORD *found_depth, UWORD *found_width, UWORD *found_height, struct GfxBase *GfxBase)
{
    struct DisplayInfoHandle *dinfo;
    struct DisplayInfo disp;
    struct DimensionInfo dims;

    if (boardname)
    {
	STRPTR name;

	OOP_GetAttr(mdd->gfxhidd, aHidd_Gfx_DriverName, (IPTR *)&name);
	if (strcmp(boardname, name))
	    return;
    }

    for (dinfo = mdd->modes; dinfo->id != vHidd_ModeID_Invalid; dinfo++)
    {
	UWORD gm_width, gm_height;
	ULONG modeid = mdd->id | dinfo->id;

	D(bug("[BestModeID] Checking ModeID 0x%08X... ", modeid));

	if (GetDisplayInfoData(dinfo, (UBYTE *)&disp, sizeof(disp), DTAG_DISP, modeid) < offsetof(struct DisplayInfo, pad2))
	{
	    D(bug("No DisplayInfo!\n"));
	    continue;
	}

	/* Filter out not available modes */
	if (disp.NotAvailable)
	{
	    D(bug("Not available: %u\n", disp.NotAvailable));
	    continue;
	}

	/* Filter out modes which do not meet out special needs */
	if (disp.PropertyFlags & dipf_mustnothave)
	{
	    D(bug("Has MustNotHave flags: 0x%08lX\n", disp.PropertyFlags));
	    continue;
	}
	if ((disp.PropertyFlags & dipf_musthave) != dipf_musthave)
	{
	    D(bug("Does not have MustHave flags: 0x%08lX\n", disp.PropertyFlags));
	    continue;
	}

	if (GetDisplayInfoData(dinfo, (UBYTE *)&dims, sizeof(dims), DTAG_DIMS, modeid) < offsetof(struct DimensionInfo, MaxOScan))
	{
	    D(bug("No DimensionInfo!\n"));
	    continue;
	}
	gm_width  = dims.Nominal.MaxX - dims.Nominal.MinX + 1;
	gm_height = dims.Nominal.MaxY - dims.Nominal.MinY + 1;
	D(bug("%ux%ux%u", gm_width, gm_height, dims.MaxDepth));

	/* FIXME: Take aspect ratio into account (nominal_width : nominal_height) */

        /* Check if mode is not worse than requested */
	if (    disp.RedBits   >= redbits
	     && disp.GreenBits >= greenbits
	     && disp.BlueBits  >= bluebits
	     && dims.MaxDepth  >= depth
	     && gm_width  >= desired_width
	     && gm_height >= desired_height)
	{
	    /* Check if this mode matches closer than the one we already found */
	    if ((dims.MaxDepth <= *found_depth) &&
	        (gm_width <= *found_width) && (gm_height <= *found_height))
	    {
		/* Remember the new mode only if something changed. This prevents unwanted
		   jumping to another display (several displays may have the same modes,
		   in this case the last display will be picked up without this check. */
		if ((dims.MaxDepth < *found_depth) || (gm_width < *found_width) || (gm_height < *found_height))
		{
		    *found_id     = modeid;
		    *found_depth  = dims.MaxDepth;
		    *found_width  = gm_width;
		    *found_height = gm_height;

		    D(bug(" Match!\n"));
		}
	    }
	}
	D(bug("\n"));
	
    } /* for (each mode) */
}

/* Looks up a DriverData corresponding to a MonitorSpec */
struct monitor_driverdata *MonitorFromSpec(struct MonitorSpec *mspc, struct GfxBase *GfxBase)
{
    struct monitor_driverdata *ret = NULL;
    struct monitor_driverdata *mdd;
    OOP_Object *drv;

    if (!mspc)
    	return NULL;

    /*
     * FIXME: NULL ms_Object will likely mean chipset MonitorSpec (they don't have 1:1 relation with sync objects)
     * Process this correctly here. Or am i wrong ?
     */
    if (!mspc->ms_Object)
    	 return NULL;

    OOP_GetAttr((OOP_Object *)mspc->ms_Object, aHidd_Sync_GfxHidd, (IPTR *)&drv);

    ObtainSemaphoreShared(&CDD(GfxBase)->displaydb_sem);

    for (mdd = CDD(GfxBase)->monitors; mdd; mdd = mdd->next)
    {
	/*
	 * Sync objects know nothing about fakegfx proxy class.
	 * They carry a pointer to a real driver object.
	 */
    	if (mdd->gfxhidd_orig == drv)
    	{
    	    ret = mdd;
    	    break;
    	}
    }

    ReleaseSemaphore(&CDD(GfxBase)->displaydb_sem);
    
    return ret;
}
