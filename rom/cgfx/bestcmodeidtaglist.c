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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

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
	{ BIDTAG_MonitorID,	INVALID_ID},
	{ TAG_DONE, 0UL }
    };

    STRPTR boardname;
 
    boardname		= "Blah";
      
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
	        modetags[3].ti_Data = tag->ti_Data;
	    	break;

	    case CYBRBIDTG_BoardName:
	    	boardname = (STRPTR)tag->ti_Data;
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
