/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/debug.h>
#include <cybergraphx/cybergraphics.h>
#include <hidd/graphics.h>
#include <proto/graphics.h>
#include <proto/oop.h>

#include "cybergraphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <clib/cybergraphics_protos.h>

	AROS_LH2(ULONG, GetCyberIDAttr,

/*  SYNOPSIS */
	AROS_LHA(ULONG, attribute, 	D0),
	AROS_LHA(ULONG, DisplayModeID, 	D1),

/*  LOCATION */
	struct Library *, CyberGfxBase, 17, Cybergraphics)

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
    
    /* First lookup the pixfmt for the ID */
    IPTR retval = (ULONG)-1;
    struct VecInfo info;

    if (GetDisplayInfoData(NULL, (UBYTE *)&info, sizeof(info), DTAG_VEC, DisplayModeID) == sizeof(info)) {
	OOP_Object *sync = (OOP_Object *)info.reserved[0];
        OOP_Object *pf   = (OOP_Object *)info.reserved[1];
        IPTR depth;

	OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
    
	if (depth < 8) {
    	    D(bug("!!! TRYING TO GET ATTR FROM NON-CGFX MODE IN GetCyberIDAttr() !!!\n"));
	    retval = (ULONG)-1;
	} else {
    
	    switch (attribute) {
		case CYBRIDATTR_PIXFMT: {
	    	    HIDDT_StdPixFmt stdpf;
		
		    OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, &stdpf);
		
		    retval = hidd2cyber_pixfmt(stdpf);
		    if (-1 == retval) {
			D(bug("!!! NO CGFX PIXFMT IN GetCyberIDAttr() !!!\n"));
		    }
	    	    break; }
	
		case CYBRIDATTR_DEPTH:
	     	    retval = depth;
		    break;
	
		case CYBRIDATTR_WIDTH:
	    	    OOP_GetAttr(sync, aHidd_Sync_HDisp, &retval);
		    break;
		
		case CYBRIDATTR_HEIGHT:
	    	    OOP_GetAttr(sync, aHidd_Sync_VDisp, &retval);
		    break;
		
		case CYBRIDATTR_BPPIX:
	    	    OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, &retval);
		    break;
		
		default:
	    	    D(bug("!!! UNKONOW ATTRIBUTE IN GetCyberIDAttr(): %x !!!\n"
			, attribute));
		    retval = (ULONG)-1;
		    break;
	    	
    	
	    }
	}
    }
    return retval;

    AROS_LIBFUNC_EXIT
} /* GetCyberIDAttr */
