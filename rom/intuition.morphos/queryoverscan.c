/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <graphics/displayinfo.h>
#include <proto/graphics.h>
#include "intuition_intern.h"

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
    LONG              	 retval = FALSE;

    DEBUG_QUERYOVERSCAN(dprintf("LIB_QueryOverscan: displayid 0x%lx rect 0x%lx oscantype 0x%lx\n",
                                displayid,
                                rect,
                                oscantype));

    DEBUG_QUERYOVERSCAN(dprintf("LIB_QueryOverscan: %ld %ld %ld %ld\n",
                                rect->MinX,
                                rect->MinY,
                                rect->MaxX,
                                rect->MaxY));
    ASSERT_VALID_PTR(rect);

    if (GetDisplayInfoData(NULL, (UBYTE *)&diminfo, sizeof(diminfo), DTAG_DIMS, displayid) > 0)
    {
        retval = TRUE;

        switch(oscantype)
        {
            case OSCAN_TEXT:
        	DEBUG_QUERYOVERSCAN(dprintf("LIB_QueryOverscan: OSCAN_TEXT\n"));
        	memcpy(rect,&diminfo.TxtOScan,sizeof(struct Rectangle));
        	break;

            case OSCAN_STANDARD:
        	DEBUG_QUERYOVERSCAN(dprintf("LIB_QueryOverscan: OSCAN_STANDARD\n"));
        	memcpy(rect,&diminfo.StdOScan,sizeof(struct Rectangle));
        	break;

            case OSCAN_MAX:
        	DEBUG_QUERYOVERSCAN(dprintf("LIB_QueryOverscan: OSCAN_MAX\n"));
        	memcpy(rect,&diminfo.MaxOScan,sizeof(struct Rectangle));
        	break;

            case OSCAN_VIDEO:
        	DEBUG_QUERYOVERSCAN(dprintf("LIB_QueryOverscan: OSCAN_VIDEO\n"));
        	memcpy(rect,&diminfo.VideoOScan,sizeof(struct Rectangle));
        	break;

            default:
        	DEBUG_QUERYOVERSCAN(dprintf("LIB_QueryOverscan: OSCAN_????\n"));
        	/* or should we assume OSCAN_TEXT? */
        	retval = FALSE;
        	break;

        } /* switch(oscantype) */

    } /* if (GetDisplayInfoData(NULL, &diminfo, sizeof(diminfo), DTAG_DIMS, displayid) > 0) */

    DEBUG_QUERYOVERSCAN(dprintf("LIB_QueryOverscan: retval %ld, %ld %ld %ld %ld\n",
                                retval,
                                rect->MinX,
                                rect->MinY,
                                rect->MaxX,
                                rect->MaxY));

    return retval;

    AROS_LIBFUNC_EXIT

} /* QueryOverscan */
