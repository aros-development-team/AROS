/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/debug.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include "cybergraphics_intern.h"

static STRPTR ids[] = {
    NULL,
    "CVision64",
    "Piccolo",
    "PicassoII",
    "Spectrum",
    "Domino",
    "RetinaZ3",
    "PiccoSD64",
    "A2410",
    NULL,
    NULL,
    NULL,
    NULL,
    "CVision3D",
    "Inferno",
    "PicassoIV"
};

#define MAX_ID 15

/*****************************************************************************

    NAME */
#include <proto/cybergraphics.h>

	AROS_LH1(ULONG, BestCModeIDTagList,

/*  SYNOPSIS */
	AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
	struct Library *, CyberGfxBase, 10, Cybergraphics)

/*  FUNCTION
	Finds best RTG display mode ID which matches parameters specified
	by the taglist.

    INPUTS
	tags - An optional pointer to a TagList containint requirements
	       for the display mode. Valid tags are:

	  CYBRBIDTG_Depth (ULONG) - depth the returned ModeID must support.
				    Defaults to 8.
	  CYBRBIDTG_NominalWidth  (UWORD),
	  CYBRBIDTG_NominalHeight (UWORD) - desired width and height for the
                                            display mode.
          CYBRBIDTG_MonitorID (ULONG) - Specify numeric driver ID to find only
					modes belonging to this driver. Useful
					for systems with several graphics cards.
					Defined board IDs are:
					  1 - CVision64
					  2 - Piccolo
					  3 - PicassoII
					  4 - Spectrum
					  5 - Domino
					  6 - RetinaZ3
					  7 - PiccoSD64
                                          8 - A2410
					 13 - CVision3D (V41)
					 14 - Inferno   (V41)
					 15 - PicassoIV (V41)
					Note that this tag exists only for
					compatibility with old software. New
					programs should use CYBRIDTG_BoardName
					tag instead.
	CYBRBIDTG_BoardName (STRPTR) - Specify the driver name directly. For
				       example, pass "CVision3D" to get a
				       CyberVision64/3D display mode ID

    RESULT
	Best matchind display mode ID or INVALID_ID if there is no match

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	graphics.library/BestModeIDA()

    INTERNALS
	This functions relies on processing CYBRIDTG_BoardName tag by
	graphics.library/BestModeIDA()

    HISTORY
	27-11-96    digulla automatically created from
			    cybergraphics_lib.fd and clib/cybergraphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tstate;
    struct TagItem *tag;
    struct TagItem modetags[] = {
	{ BIDTAG_NominalWidth  , 800	},
	{ BIDTAG_NominalHeight , 600	},
	{ BIDTAG_Depth         , 8	},
	{ CYBRBIDTG_BoardName  , 0	},
	{ TAG_DONE             , 0	}
    };
      
    for (tstate = tags; (tag = NextTagItem(&tstate)); ) {
    	switch (tag->ti_Tag) {
	    case CYBRBIDTG_Depth:
	    	modetags[2].ti_Data = tag->ti_Data;
	    	break;
	
	    case CYBRBIDTG_NominalWidth:
	        modetags[0].ti_Data = tag->ti_Data;
	    	break;

	    case CYBRBIDTG_NominalHeight:
	        modetags[1].ti_Data = tag->ti_Data;
	    	break;

	    case CYBRBIDTG_MonitorID:
		if (tag->ti_Data > MAX_ID)
		    return INVALID_ID;
		modetags[3].ti_Data = (IPTR)ids[tag->ti_Data];
	    	break;

	    case CYBRBIDTG_BoardName:
		modetags[3].ti_Data = tag->ti_Data;
	    	break;

	    default:
	    	D(bug("!!! UNKOWN ATTR PASSED TO BestCModeIDTagList(): %x !!!\n", tag->ti_Tag));
		break;

	} /* switch () */
      
    } /* for (each tag in the taglist) */
    
    /* Get the best modeid */
    return BestModeIDA(modetags);

    AROS_LIBFUNC_EXIT
} /* BestCModeIDTagList */
