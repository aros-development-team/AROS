/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition function QueryOverscan()
    Lang: english
*/
#include "intuition_intern.h"
#include <graphics/displayinfo.h>
#include <proto/graphics.h>

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH3(LONG, QueryOverscan,

/*  SYNOPSIS */
	AROS_LHA(ULONG             , displayid, A0),
	AROS_LHA(struct Rectangle *, rect     , A1),
	AROS_LHA(WORD              , oscantype, D0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 79, Intuition)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct DimensionInfo diminfo;
    LONG		 retval = FALSE;
    
    ASSERT_VALID_PTR(rect);
    
    if (GetDisplayInfoData(NULL, (UBYTE *)&diminfo, sizeof(diminfo), DTAG_DIMS, displayid) > 0)
    {
        retval = TRUE;

        switch(oscantype)
	{
	    case OSCAN_TEXT:
	        *rect = diminfo.TxtOScan;
		break;
		
	    case OSCAN_STANDARD:
	        *rect = diminfo.StdOScan;
		break;
		
	    case OSCAN_MAX:
	        *rect = diminfo.MaxOScan;
		break;
		
	    case OSCAN_VIDEO:
	        *rect = diminfo.VideoOScan;
		break;
		
	    default:
	        /* or should we assume OSCAN_TEXT? */
	        retval = FALSE;
		break;
		
	} /* switch(oscantype) */
	
    } /* if (GetDisplayInfoData(NULL, &diminfo, sizeof(diminfo), DTAG_DIMS, displayid) > 0) */

    return retval;

    AROS_LIBFUNC_EXIT
    
} /* QueryOverscan */
