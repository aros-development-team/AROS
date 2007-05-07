/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function BestModeIDA()
    Lang: english
*/
#include <aros/debug.h>
#include <graphics/modeid.h>
#include <hidd/graphics.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include "dispinfo.h"
#include "graphics_intern.h"

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

    RESULT
        ID - ID of the best mode to use, or INVALID_ID if a match
             could not be found

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        graphics/modeid.h graphics/displayinfo.h

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

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
    vp			= (struct ViewPort *)GetTagData(BIDTAG_ViewPort, (IPTR)NULL	, TagItems);
    monitorid		= GetTagData(BIDTAG_MonitorID		, monitorid		, TagItems);
    sourceid		= GetTagData(BIDTAG_SourceID		, sourceid		, TagItems);
    depth		= GetTagData(BIDTAG_Depth		, depth			, TagItems);
    nominal_width	= GetTagData(BIDTAG_NominalWidth	, nominal_width		, TagItems);
    nominal_height	= GetTagData(BIDTAG_NominalHeight	, nominal_height	, TagItems);
    desired_width	= GetTagData(BIDTAG_DesiredWidth	, desired_width		, TagItems);
    desired_height	= GetTagData(BIDTAG_DesiredHeight	, desired_height	, TagItems);
    redbits		= GetTagData(BIDTAG_RedBits		, redbits		, TagItems);
    greenbits		= GetTagData(BIDTAG_GreenBits		, greenbits		, TagItems);
    bluebits		= GetTagData(BIDTAG_BlueBits		, bluebits		, TagItems);
    dipf_musthave	= GetTagData(BIDTAG_DIPFMustHave	, dipf_musthave		, TagItems);
    dipf_mustnothave	= GetTagData(BIDTAG_DIPFMustNotHave	, dipf_mustnothave	, TagItems);
    
    if (NULL != vp)
    {
    	/* Set some new default values */
	nominal_width  = desired_width  = vp->DWidth;
	nominal_height = desired_height = vp->DHeight;
	
	if (NULL != vp->RasInfo->BitMap)
	{
	    depth = GetBitMapAttr(vp->RasInfo->BitMap, BMA_DEPTH);
	}
	else
	{
	    D(bug("!!! Passing viewport with NULL vp->RasInfo->BitMap to BestModeIDA() !!!\n"));
	}
    }
    
    if (INVALID_ID != sourceid)
    {
#warning Fix this

/* I do not understand what the docs state about this */
	
    }
    
    /* OK, now we try to search for a mode that has the supplied charateristics */
    
    hiddmode = vHidd_ModeID_Invalid;
    for (;;)
    {
	OOP_Object *sync, *pf;
	
	ULONG redmask, greenmask, bluemask;
	ULONG gm_depth, gm_width, gm_height;
	ULONG found_depth, found_width, found_height;
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
	     && gm_height >= desired_height)
	{
#warning Fix this 	    
	    	/* We return the first modeid that fulfill the criterias.
	 	      Instead we should find the mode that has:
		           - largest possible depth.
	    	*/
	    if (
		    (found_id == INVALID_ID) ||
		    (
			(found_id != INVALID_ID) &&
			(gm_depth < found_depth) &&
			(gm_width < found_width) &&
			(gm_height < found_height)
		    )
		)
	    {
		found_id = HIDD_TO_AMIGA_MODEID(hiddmode);
		found_depth = gm_depth;
		found_width = gm_width;
		found_height = gm_height;
	    }
	} 
	
    } /* for (each HIDD modeid) */
    
    return found_id;

    AROS_LIBFUNC_EXIT
} /* BestModeIDA */
