/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function BestModeIDA()
    Lang: english
*/

#include <aros/debug.h>
#include <cybergraphx/cybergraphics.h>
#include <graphics/modeid.h>
#include <hidd/graphics.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <stddef.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"
#include "dispinfo.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH1(ULONG, BestModeIDA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, TagItems, A0),

/*  LOCATION */
        struct GfxBase *, GfxBase, 175, Graphics)

/*  FUNCTION

    INPUTS
        TagItems - pointer to an array of TagItems

    TAGS
	BIDTAG_ViewPort (struct ViewPort *) - Viewport for which a mode is searched. Default: NULL
	BIDTAG_MonitorID (ULONG)            - Returned ID must use this monitor
	BIDTAG_SourceID (ULONG)             - Use this ModeID instead of a ViewPort.
	                                      DIPFMustHave mask is made up of the
	                                      ((DisplayInfo->PropertyFlags of this ID & SPECIAL_FLAGS) |
	                                      DIPFMustHave flags).
	                                      Default:
                                              if BIDTAG_ViewPort was passed: VPModeID(vp), else the
                                              DIPFMustHave and DIPFMustNotHave are unchanged.
	BIDTAG_Depth (UBYTE)                - Minimal depth. Default:
                                              if BIDTAG_ViewPort is passed: vp->RasInfo->BitMap->Depth,
                                              else 1.
	BIDTAG_NominalWidth (UWORD),
	BIDTAG_NominalHeight (UWORD)        - Aspect radio. Default:
                                              if BIDTAG_SourceID: SourceID NominalDimensionInfo
                                              if BIDTAG_ViewPort: vp->DWidth and vp->DHeight
                                              or 640 x 200.
	BIDTAG_DesiredWidth (UWORD)         - Width. Default: DIBTAG_NominalWidth.
	BIDTAG_DesiredHeight (UWORD)        - Height. Default: BIDTAG_NominalHeight.
	BIDTAG_RedBits (UBYTE),
	BIDTAG_GreenBits (UBYTE),
	BIDTAG_BlueBits (UBYTE)             - Bits per gun the mode must support. Default: 4
	BIDTAG_DIPFMustHave (ULONG)         - DIPF flags the resulting mode must have
	BIDTAG_DIPFMustNotHave (ULONG)      - DIPF flags the resulting mode must not have
 
    RESULT
        ID - ID of the best mode to use, or INVALID_ID if a match
             could not be found

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        graphics/modeid.h, graphics/displayinfo.h

    INTERNALS
	This function also processes CYBRBIDTG_BoardName tag. This is private
	to AROS, do not rely on it!

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tag, *tstate = TagItems;
    UWORD desired_width, desired_height;
    struct DisplayInfoHandle *dinfo;
    struct DisplayInfo disp;
    struct DimensionInfo dims;
    struct monitor_driverdata *monitor;
    struct ViewPort *vp    = NULL;
    ULONG dipf_musthave    = 0;
    ULONG dipf_mustnothave = SPECIAL_FLAGS;
    ULONG monitorid        = INVALID_ID;
    UBYTE redbits          = 4;
    UBYTE greenbits        = 4;
    UBYTE bluebits         = 4;
    ULONG sourceid         = INVALID_ID;
    UWORD nominal_width    = 640;
    UWORD nominal_height   = 200;
    UBYTE depth            = 1;
    STRPTR boardname       = NULL;
    ULONG found_id         = INVALID_ID;
    UWORD found_depth      = -1;
    UWORD found_width      = -1;
    UWORD found_height     = -1;

    /* Obtain default monitor driver */
    monitor = MonitorFromSpec(GfxBase->default_monitor, GfxBase);

    /* Get defaults which can be overriden */
    while ((tag = NextTagItem(&tstate)))
    {
    	switch (tag->ti_Tag)
    	{
    	case BIDTAG_DIPFMustHave:
    	    dipf_musthave = tag->ti_Data;
    	    break;

    	case BIDTAG_DIPFMustNotHave:
    	    dipf_mustnothave = tag->ti_Data;
    	    break;

	case BIDTAG_MonitorID:
	    monitorid = tag->ti_Data;
	    break;

	case BIDTAG_RedBits:
	    redbits = tag->ti_Data;
	    break;

	case BIDTAG_BlueBits:
	    bluebits = tag->ti_Data;
	    break;

	case BIDTAG_ViewPort:
	    /* If we got ViewPort, obtain some more defaults from it */
	    vp = (struct ViewPort *)tag->ti_Data;
	    nominal_width  = vp->DWidth;
	    nominal_height = vp->DHeight;
	    sourceid       = GetVPModeID(vp);
	    depth          = GET_BM_DEPTH(vp->RasInfo->BitMap);
	    monitor	   = GET_VP_DRIVERDATA(vp);
	    break;

	/* Offer some help to cybergraphics.library */
	case CYBRBIDTG_BoardName:
	    boardname = (STRPTR)tag->ti_Data;
	    break;
	}
    }

    /* Then process SourceID, it overrides ViewPort size and mode and specifies current monitor */
    sourceid = GetTagData(BIDTAG_SourceID, sourceid, TagItems);
    if (sourceid != INVALID_ID)
    {
	/* Patch musthave flags */
        if (GetDisplayInfoData(NULL, (UBYTE *)&disp, sizeof(disp), DTAG_DISP, sourceid) >= offsetof(struct DisplayInfo, Resolution))
	    dipf_musthave |= (disp.PropertyFlags & SPECIAL_FLAGS);

	if (!vp)
	{
	    /* Override monitor and nominal size from source ID only if there was no ViewPort specified */
	    dinfo = FindDisplayInfo(sourceid);
	    if (dinfo)
	    	monitor = dinfo->drv;

	    if (GetDisplayInfoData(dinfo, (UBYTE *)&dims, sizeof(dims), DTAG_DIMS, sourceid) >= offsetof(struct DimensionInfo, MaxOScan))
	    {
	    	nominal_width  = dims.Nominal.MaxX - dims.Nominal.MinX + 1;
	    	nominal_height = dims.Nominal.MaxY - dims.Nominal.MinY + 1;
	    }
	}
    }

    /* Get high-priority parameters */
    nominal_width  = GetTagData(BIDTAG_NominalWidth , nominal_width , TagItems);
    nominal_height = GetTagData(BIDTAG_NominalHeight, nominal_height, TagItems);
    desired_width  = GetTagData(BIDTAG_DesiredWidth , nominal_width , TagItems);
    desired_height = GetTagData(BIDTAG_DesiredHeight, nominal_height, TagItems);
    depth	   = GetTagData(BIDTAG_Depth        , depth         , TagItems);

    /* Exclude flags in MustHave from MustNotHave (CHECKME: if this correct?) */
    dipf_mustnothave &= ~dipf_musthave;
    /* Mask out bit 12 in monitorid because the user may (and will) pass in IDs defined in include/graphics/modeid.h
       (like PAL_MONITOR_ID, VGA_MONITOR_ID, etc) which have bit 12 set) */
    if (monitorid != INVALID_ID)
        monitorid &= AROS_MONITOR_ID_MASK;

    D(bug("[BestModeIDA] Desired mode: %dx%dx%d, MonitorID 0x%08lX, MustHave 0x%08lX, MustNotHave 0x%08lX\n",
	  desired_width, desired_height, depth, monitorid, dipf_musthave, dipf_mustnothave));

    /* OK, now we try to search for a mode that has the supplied charateristics. */
    ObtainSemaphoreShared(&CDD(GfxBase)->displaydb_sem);

    /* First we try to search in preferred monitor (if present) */
    if (monitor)
    	BestModeIDForMonitor(monitor, dipf_musthave, dipf_mustnothave, redbits, greenbits, bluebits,
    			     depth, boardname, nominal_width, nominal_height, desired_width, desired_height,
    			     &found_id, &found_depth, &found_width, &found_height, GfxBase);

    /* And if nothing was found there, check other monitors */
    if (found_id == INVALID_ID)
    {
    	struct monitor_driverdata *mdd;

    	for (mdd = CDD(GfxBase)->monitors; mdd; mdd = mdd->next)
    	{
    	    if (mdd != monitor)
	    	BestModeIDForMonitor(mdd, dipf_musthave, dipf_mustnothave, redbits, greenbits, bluebits,
    				     depth, boardname, nominal_width, nominal_height, desired_width, desired_height,
    				     &found_id, &found_depth, &found_width, &found_height, GfxBase);
    	}
    }

    ReleaseSemaphore(&CDD(GfxBase)->displaydb_sem);

    D(bug("[BestModeIDA] Returning mode ID 0x%08lX\n", found_id));
    return found_id;

    AROS_LIBFUNC_EXIT
} /* BestModeIDA */
