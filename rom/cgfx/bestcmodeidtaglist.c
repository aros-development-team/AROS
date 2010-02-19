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
#include <clib/cybergraphics_protos.h>

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
      
    ULONG nominal_width, nominal_height, depth;
    ULONG monitorid;
    STRPTR boardname;
    ULONG modeid;
      
    nominal_width	= 800;
    nominal_height	= 600;
    depth		= 8;
    monitorid		= 0;
    boardname		= "Blah";
      
    for (tstate = tags; (tag = NextTagItem(&tstate)); ) {
    	switch (tag->ti_Tag) {
	    case CYBRBIDTG_Depth:
	    	depth = tag->ti_Data;
	    	break;
	
	    case CYBRBIDTG_NominalWidth:
	        nominal_width = tag->ti_Data;
	    	break;

	    case CYBRBIDTG_NominalHeight:
	        nominal_height = tag->ti_Data;
	    	break;

	    case CYBRBIDTG_MonitorID:
	        monitorid = tag->ti_Data;
	    	break;

	    case CYBRBIDTG_BoardName:
	    	boardname = (STRPTR)tag->ti_Data;
	    	break;
		
	    default:
	    	D(bug("!!! UNKOWN ATTR PASSED TO BestCModeIDTagList(): %x !!!\n", tag->ti_Tag));
		break;

	} /* switch () */
      
    } /* for (each tag in the taglist) */
    
    if (depth < 8 )  {
    
    	/* No request for a cgfx mode */
    	modeid = INVALID_ID;
    
    } else {
	/* Get the best modeid */
	struct TagItem modetags[] = {
	    { BIDTAG_NominalWidth,	nominal_width	},
	    { BIDTAG_NominalHeight,	nominal_height	},
	    { BIDTAG_DesiredWidth,	nominal_width	},
	    { BIDTAG_DesiredHeight,	nominal_height	},
	    { BIDTAG_Depth,		depth		},
	    { BIDTAG_MonitorID,		monitorid	},
	    { TAG_DONE, 0UL }
	};
	
	modeid = BestModeIDA(modetags);
    }
    
    /* Use the data to select a mode */
    return modeid;

    AROS_LIBFUNC_EXIT
} /* BestCModeIDTagList */
