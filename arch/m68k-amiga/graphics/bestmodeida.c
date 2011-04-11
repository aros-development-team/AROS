/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: bestmodeida.c 37793 2011-03-26 21:40:53Z verhaegs $

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

    struct ViewPort *vp;
    UWORD nominal_width, nominal_height;
    UWORD desired_width, desired_height;
    UBYTE depth;
    ULONG sourceid, monitorid;
    UBYTE redbits, greenbits, bluebits;
    ULONG dipf_musthave, dipf_mustnothave;
    ULONG modeid;
    STRPTR boardname;
    struct DisplayInfoHandle *dinfo;
    struct DisplayInfo disp;
    struct DimensionInfo dims;
    ULONG found_id     = INVALID_ID;
    UWORD found_depth  = -1;
    UWORD found_width  = -1;
    UWORD found_height = -1;
    UBYTE round;

    /* Get defaults which can be overriden */
    dipf_musthave    = GetTagData(BIDTAG_DIPFMustHave   , 0		, TagItems);
    dipf_mustnothave = GetTagData(BIDTAG_DIPFMustNotHave, SPECIAL_FLAGS	, TagItems);
    monitorid	     = GetTagData(BIDTAG_MonitorID      , INVALID_ID	, TagItems);
    redbits	     = GetTagData(BIDTAG_RedBits        , 4		, TagItems);
    greenbits	     = GetTagData(BIDTAG_GreenBits      , 4		, TagItems);
    bluebits	     = GetTagData(BIDTAG_BlueBits       , 4		, TagItems);

    /* Try to get viewport */
    vp = (struct ViewPort *)GetTagData(BIDTAG_ViewPort, 0, TagItems);
    if (vp) {
        nominal_width  = vp->DWidth;
	nominal_height = vp->DHeight;
	sourceid = GetVPModeID(vp);
	depth = vp->RasInfo->BitMap->Depth;
    } else {
        sourceid       = INVALID_ID;
	nominal_width  = 640;
	nominal_height = 200;
	depth          = 1;
    }

    /* Then process SourceID, it overrides ViewPort size and mode */
    sourceid = GetTagData(BIDTAG_SourceID, sourceid, TagItems);
    if (sourceid != INVALID_ID) {
        if (GetDisplayInfoData(NULL, (UBYTE *)&disp, sizeof(disp), DTAG_DISP, sourceid) >= offsetof(struct DisplayInfo, Resolution))
	    dipf_musthave |= (disp.PropertyFlags & SPECIAL_FLAGS);
	
	if (GetDisplayInfoData(NULL, (UBYTE *)&dims, sizeof(dims), DTAG_DIMS, sourceid) >= offsetof(struct DimensionInfo, MaxOScan)) {
	    nominal_width  = dims.Nominal.MaxX - dims.Nominal.MinX + 1;
	    nominal_height = dims.Nominal.MaxY - dims.Nominal.MinY + 1;
	}
    }

    /* Get high-priority parameters */
    nominal_width  = GetTagData(BIDTAG_NominalWidth , nominal_width , TagItems);
    nominal_height = GetTagData(BIDTAG_NominalHeight, nominal_height, TagItems);
    desired_width  = GetTagData(BIDTAG_DesiredWidth , nominal_width , TagItems);
    desired_height = GetTagData(BIDTAG_DesiredHeight, nominal_height, TagItems);
    depth	   = GetTagData(BIDTAG_Depth        , depth         , TagItems);

    boardname = (STRPTR)GetTagData(CYBRBIDTG_BoardName, 0, TagItems);

    /* Exclude flags in MustHave from MustNotHave (CHECKME: if this correct?) */
    dipf_mustnothave &= ~dipf_musthave;
    /* Mask out bit 12 in monitorid because the user may (and will) pass in IDs defined in include/graphics/modeid.h
       (like PAL_MONITOR_ID, VGA_MONITOR_ID, etc) which have bit 12 set) */
    if (monitorid != INVALID_ID)
        monitorid &= AROS_MONITOR_ID_MASK;

    D(bug("[BestModeIDA] Desired mode: %dx%dx%d, MonitorID 0x%08lX, MustHave 0x%08lX, MustNotHave 0x%08lX\n",
	  desired_width, desired_height, depth, monitorid, dipf_musthave, dipf_mustnothave));

    round = 0;
    /* OK, now we try to search for a mode that has the supplied charateristics */
    for (modeid = INVALID_ID;;)
    {
	UWORD gm_width, gm_height;

        modeid = NextDisplayInfo(modeid);
	if (modeid == INVALID_ID) {
	    round++;
	    if (round == 2)
	    	break;
	    continue;
	}
	/* scan RTG modes first */
	if (round == 0 && (modeid & 0xf0000000) == 0)
	    continue;
	/* scan native modes next only if nothing was found */
	if (round == 1 && found_id != INVALID_ID)
	    break;
	/* scan chipset modes last */
	if (round == 1 && (modeid & 0xf0000000) != 0)
	    continue;

	D(bug("[BestModeIDA] Checking ModeID 0x%08lX... ", modeid));

	if ((monitorid != INVALID_ID) && ((modeid & AROS_MONITOR_ID_MASK) != monitorid)) {
	    D(bug("MonitorID does not match\n"));
	    continue;
	}

	dinfo = FindDisplayInfo(modeid);
	if (!dinfo) {
	    D(bug("No DisplayInfoHandle!\n"));
	    continue;
	}

	if (boardname) {
	    STRPTR name;

	    OOP_GetAttr(dinfo->drv->gfxhidd, aHidd_Gfx_DriverName, (IPTR *)&name);
	    if (strcmp(boardname, name))
		continue;
	}

	if (GetDisplayInfoData(dinfo, (UBYTE *)&disp, sizeof(disp), DTAG_DISP, modeid) < offsetof(struct DisplayInfo, pad2)) {
	    D(bug("No DisplayInfo!\n"));
	    continue;
	}
	if (disp.NotAvailable) { /* Filter out not available modes */
	    D(bug("Not available: %u\n", disp.NotAvailable));
	    continue;
	}
	if (disp.PropertyFlags & dipf_mustnothave) { /* Filter out modes which do not meet out special needs */
	    D(bug("Has MustNotHave flags: 0x%08lX\n", disp.PropertyFlags));
	    continue;
	}
	if ((disp.PropertyFlags & dipf_musthave) != dipf_musthave) {
	    D(bug("Does not have MustHave flags: 0x%08lX\n", disp.PropertyFlags));
	    continue;
	}

	if (GetDisplayInfoData(dinfo, (UBYTE *)&dims, sizeof(dims), DTAG_DIMS, modeid) < offsetof(struct DimensionInfo, MaxOScan)) {
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
	    if ((dims.MaxDepth <= found_depth) &&
	        (gm_width <= found_width) && (gm_height <= found_height))
	    {
		/* Remember the new mode only if something changed. This prevents unwanted
		   jumping to another display (several displays may have the same modes,
		   in this case the last display will be picked up without this check. */
		if ((dims.MaxDepth < found_depth) || (gm_width < found_width) || (gm_height < found_height))
		{
		    found_id     = modeid;
		    found_depth  = dims.MaxDepth;
		    found_width  = gm_width;
		    found_height = gm_height;
		    D(bug(" Match!\n"));
		}
	    }
	}
	D(bug("\n"));
	
    } /* for (each modeid) */

    D(bug("[BestModeIDA] Returning mode ID 0x%08lX\n", found_id));
    return found_id;

    AROS_LIBFUNC_EXIT
} /* BestModeIDA */
