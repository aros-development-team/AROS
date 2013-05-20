/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <hidd/graphics.h>
#include <proto/graphics.h>
#include <proto/oop.h>

#include "cybergraphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <proto/cybergraphics.h>

	AROS_LH1(BOOL, IsCyberModeID,

/*  SYNOPSIS */
	AROS_LHA(ULONG, modeID, D0),

/*  LOCATION */
	struct Library *, CyberGfxBase, 9, Cybergraphics)

/*  FUNCTION
	Checks if the given display mode ID belongs to an RTG driver.

    INPUTS
	modeID - a display mode ID to check.

    RESULT
        result - TRUE if the mode belongs to an RTG driver, FALSE otherwise.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
    	The function relies on the pixelformat object being passed in
        DimensionInfo.reserved[1] by graphics.library/GetDisplayInfoData().

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct DimensionInfo info;

    /* This function works by querying pixelformat for the mode and checking if it is planar */
    if (GetDisplayInfoData(NULL, (UBYTE *)&info, sizeof(info), DTAG_DIMS, modeID) == sizeof(info)) {
    	HIDDT_StdPixFmt stdpf;
	OOP_Object *pf = (OOP_Object *)info.reserved[1];

	OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, &stdpf);

	return (stdpf != vHidd_StdPixFmt_Plane);
    }

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* IsCyberModeID */
