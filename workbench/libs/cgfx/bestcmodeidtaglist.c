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
	tags - A pointer to a TagList

    RESULT
	Best matchind display mode ID or INVALID_ID if there is no match

    NOTES

    EXAMPLE

    BUGS
	At the moment MonitorID and BoardName tags are not implemented

    SEE ALSO
	graphics.library/BestModeIDA()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    cybergraphics_lib.fd and clib/cybergraphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    const struct TagItem *tstate;
    struct TagItem *tag;
    struct TagItem modetags[] = {
	{ BIDTAG_NominalWidth,	800	},
	{ BIDTAG_NominalHeight,	600	},
	{ BIDTAG_Depth,		8	},
	{ TAG_DONE, 0UL }
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
/* FIXME: These two need to be implemented
	    case CYBRBIDTG_MonitorID:
	    	break;

	    case CYBRBIDTG_BoardName:
	    	break;*/
		
	    default:
	    	D(bug("!!! UNKOWN ATTR PASSED TO BestCModeIDTagList(): %x !!!\n", tag->ti_Tag));
		break;

	} /* switch () */
      
    } /* for (each tag in the taglist) */
    
    /* Get the best modeid */
    return BestModeIDA(modetags);

    AROS_LIBFUNC_EXIT
} /* BestCModeIDTagList */
