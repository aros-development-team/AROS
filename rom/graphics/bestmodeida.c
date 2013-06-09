/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
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

/*
 * There are lots of parameters and it's totally inoptimal to pass them
 * as separate arguments.
 * Instead we store our search state as a structure.
 */
struct MatchData
{
    /* Arguments */
    ULONG  dipf_musthave;
    ULONG  dipf_mustnothave;
    UBYTE  redbits;
    UBYTE  greenbits;
    UBYTE  bluebits;
    UBYTE  depth;
    ULONG  monitorid;
    STRPTR boardname;
    UWORD  nominal_width;
    UWORD  nominal_height;
    UWORD  desired_width;
    UWORD  desired_height;
    /* Results */
    ULONG  found_id;
    UWORD  found_depth;
    UWORD  found_width;
    UWORD  found_height;
};

static void BestModeIDForMonitor(struct monitor_driverdata *mdd, struct MatchData *args, struct GfxBase *GfxBase)
{
    struct DisplayInfoHandle *dinfo;
    struct DisplayInfo disp;
    struct DimensionInfo dims;

    if (args->boardname)
    {
	STRPTR name;

	OOP_GetAttr(mdd->gfxhidd, aHidd_Gfx_DriverName, (IPTR *)&name);
	if (strcmp(args->boardname, name))
	    D(bug("CYBRBIDTG_BoardName didn't match. '%s' != '%s'\n", args->boardname, name));
	    return;
    }

    for (dinfo = mdd->modes; dinfo->id != vHidd_ModeID_Invalid; dinfo++)
    {
	UWORD gm_width, gm_height;
	ULONG modeid = mdd->id | dinfo->id;

	D(bug("[BestModeID] Checking ModeID 0x%08X... ", modeid));

    if (args->monitorid != INVALID_ID && args->monitorid != modeid)
    {
        D(bug("BIDTAG_MonitorID 0x%08X didn't match\n", args->monitorid));
        continue;
    }

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
	if (disp.PropertyFlags & args->dipf_mustnothave)
	{
	    D(bug("Has MustNotHave flags: 0x%08lX\n", disp.PropertyFlags));
	    continue;
	}
	if ((disp.PropertyFlags & args->dipf_musthave) != args->dipf_musthave)
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
	if (    disp.RedBits   >= args->redbits
	     && disp.GreenBits >= args->greenbits
	     && disp.BlueBits  >= args->bluebits
	     && dims.MaxDepth  >= args->depth
	     && gm_width       >= args->desired_width
	     && gm_height      >= args->desired_height)
	{
	    /* Check if this mode matches closer than the one we already found */
	    if ((dims.MaxDepth <= args->found_depth) &&
	        (gm_width <= args->found_width) && (gm_height <= args->found_height))
	    {
		/* Remember the new mode only if something changed. This prevents unwanted
		   jumping to another display (several displays may have the same modes,
		   in this case the last display will be picked up without this check. */
		if ((dims.MaxDepth < args->found_depth) ||
                    (gm_width < args->found_width) || (gm_height < args->found_height))
		{
		    args->found_id     = modeid;
		    args->found_depth  = dims.MaxDepth;
		    args->found_width  = gm_width;
		    args->found_height = gm_height;

		    D(bug(" Match!\n"));
		}
	    }
	}
	D(bug("\n"));
	
    } /* for (each mode) */
}

static BOOL FindBestModeIDForMonitor(struct monitor_driverdata *monitor, struct MatchData *args, struct GfxBase *GfxBase)
{
    /* First we try to search in preferred monitor (if present) */
    if (monitor)
        BestModeIDForMonitor(monitor, args, GfxBase);

    /* And if nothing was found there, check other monitors */
    if (args->found_id == INVALID_ID)
    {
        struct monitor_driverdata *mdd;

        for (mdd = CDD(GfxBase)->monitors; mdd; mdd = mdd->next)
        {
            if (mdd != monitor)
                BestModeIDForMonitor(mdd, args, GfxBase);
        }
    }
    return args->found_id != INVALID_ID;
}


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
    struct DisplayInfoHandle *dinfo;
    struct DisplayInfo disp;
    struct DimensionInfo dims;
    struct monitor_driverdata *monitor;
    struct ViewPort *vp = NULL;
    ULONG  sourceid = INVALID_ID;
    struct MatchData args =
    {
        0, SPECIAL_FLAGS, /* DIPF         */
        4, 4, 4,          /* RGB bits     */
        1,                /* Depth        */
        INVALID_ID,       /* Monitor ID   */
        NULL,             /* Board name   */
        640, 480,         /* Nominal size */
        0, 0,             /* Desired size */
        INVALID_ID,       /* Found ID     */
        -1,               /* Found depth  */
        -1, -1            /* Found size   */
    };

    /* Obtain default monitor driver */
    monitor = MonitorFromSpec(GfxBase->default_monitor, GfxBase);

    /* Get defaults which can be overriden */
    while ((tag = NextTagItem(&tstate)))
    {
    	switch (tag->ti_Tag)
    	{
    	case BIDTAG_DIPFMustHave:
    	    args.dipf_musthave = tag->ti_Data;
    	    break;

    	case BIDTAG_DIPFMustNotHave:
    	    args.dipf_mustnothave = tag->ti_Data;
    	    break;

	case BIDTAG_MonitorID:
	    args.monitorid = tag->ti_Data;
	    break;

	case BIDTAG_RedBits:
	    args.redbits = tag->ti_Data;
	    break;

	case BIDTAG_BlueBits:
	    args.bluebits = tag->ti_Data;
	    break;

	case BIDTAG_ViewPort:
	    /* If we got ViewPort, obtain some more defaults from it */
	    vp = (struct ViewPort *)tag->ti_Data;
	    args.nominal_width  = vp->DWidth;
	    args.nominal_height = vp->DHeight;
	    args.depth          = GET_BM_DEPTH(vp->RasInfo->BitMap);
	    sourceid            = GetVPModeID(vp);
	    monitor             = GET_VP_DRIVERDATA(vp);
	    break;

	/* Offer some help to cybergraphics.library */
	case CYBRBIDTG_BoardName:
	    args.boardname = (STRPTR)tag->ti_Data;
	    break;
	}
    }

    /* Then process SourceID, it overrides ViewPort size and mode and specifies current monitor */
    sourceid = GetTagData(BIDTAG_SourceID, sourceid, TagItems);
    if (sourceid != INVALID_ID)
    {
	/* Patch musthave flags */
        if (GetDisplayInfoData(NULL, (UBYTE *)&disp, sizeof(disp), DTAG_DISP, sourceid) >= offsetof(struct DisplayInfo, Resolution))
	    args.dipf_musthave |= (disp.PropertyFlags & SPECIAL_FLAGS);

	if (!vp)
	{
	    /* Override monitor and nominal size from source ID only if there was no ViewPort specified */
	    dinfo = FindDisplayInfo(sourceid);
	    if (dinfo)
	    	monitor = dinfo->drv;

	    if (GetDisplayInfoData(dinfo, (UBYTE *)&dims, sizeof(dims), DTAG_DIMS, sourceid) >= offsetof(struct DimensionInfo, MaxOScan))
	    {
	    	args.nominal_width  = dims.Nominal.MaxX - dims.Nominal.MinX + 1;
	    	args.nominal_height = dims.Nominal.MaxY - dims.Nominal.MinY + 1;
	    }
	}
    }

    /* Get high-priority parameters */
    args.nominal_width  = GetTagData(BIDTAG_NominalWidth , args.nominal_width , TagItems);
    args.nominal_height = GetTagData(BIDTAG_NominalHeight, args.nominal_height, TagItems);
    args.desired_width  = GetTagData(BIDTAG_DesiredWidth , args.nominal_width , TagItems);
    args.desired_height = GetTagData(BIDTAG_DesiredHeight, args.nominal_height, TagItems);
    args.depth          = GetTagData(BIDTAG_Depth        , args.depth         , TagItems);

    /* Exclude flags in MustHave from MustNotHave (CHECKME: if this correct?) */
    args.dipf_mustnothave &= ~args.dipf_musthave;

    D(bug("[BestModeIDA] Desired mode: %dx%dx%d, MonitorID 0x%08lX, MustHave 0x%08lX, MustNotHave 0x%08lX\n",
	  args.desired_width, args.desired_height, args.depth, args.monitorid, args.dipf_musthave, args.dipf_mustnothave));

    /* OK, now we try to search for a mode that has the supplied charateristics. */
    ObtainSemaphoreShared(&CDD(GfxBase)->displaydb_sem);

    /* First try to find exact match */
    if (!FindBestModeIDForMonitor(monitor, &args, GfxBase)) {
        /* Mask out bit 12 in monitorid because the user may (and will) pass in IDs defined in include/graphics/modeid.h
           (like PAL_MONITOR_ID, VGA_MONITOR_ID, etc) which have bit 12 set) */
        if (args.monitorid != INVALID_ID && (args.monitorid & ~AROS_MONITOR_ID_MASK)) {
            args.monitorid &= AROS_MONITOR_ID_MASK;
            FindBestModeIDForMonitor(monitor, &args, GfxBase);
        }
    }

    ReleaseSemaphore(&CDD(GfxBase)->displaydb_sem);

    D(bug("[BestModeIDA] Returning mode ID 0x%08lX\n", args.found_id));
    return args.found_id;

    AROS_LIBFUNC_EXIT
} /* BestModeIDA */
