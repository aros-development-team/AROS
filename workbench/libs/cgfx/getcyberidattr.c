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
	The function relies on pixelformat object being passed in DimensionInfo.reserved[1]
	by graphics.library/GetDisplayInfoData()

    HISTORY
	27-11-96    digulla automatically created from
			    cybergraphics_lib.fd and clib/cybergraphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG retval = (ULONG)-1;
    IPTR tmp;
    struct DimensionInfo info;

    if (GetDisplayInfoData(NULL, (UBYTE *)&info, sizeof(info), DTAG_DIMS, DisplayModeID) == sizeof(info)) {
    	OOP_Object *pf = (OOP_Object *)info.reserved[1];

	switch (attribute) {
	case CYBRIDATTR_PIXFMT: {
	    HIDDT_StdPixFmt stdpf;

	    OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, &stdpf);

	    retval = hidd2cyber_pixfmt[stdpf];
	    D(if (-1 == retval) bug("!!! NO CGFX PIXFMT IN GetCyberIDAttr() !!!\n");)
	    break;
	}

	case CYBRIDATTR_DEPTH:
	    retval = info.MaxDepth;
	    break;

	case CYBRIDATTR_WIDTH:
	    retval = info.Nominal.MaxX - info.Nominal.MinX + 1;
	    break;

	case CYBRIDATTR_HEIGHT:
	    retval = info.Nominal.MaxY - info.Nominal.MinY + 1;
	    break;

	case CYBRIDATTR_BPPIX:
	    OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, &tmp);
	    retval = tmp;
	    break;

	default:
	    D(bug("!!! UNKONOW ATTRIBUTE IN GetCyberIDAttr(): %x !!!\n", attribute));
	    break;
	}
    }
    return retval;

    AROS_LIBFUNC_EXIT
} /* GetCyberIDAttr */
