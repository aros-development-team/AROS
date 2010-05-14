/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
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
#include <clib/cybergraphics_protos.h>

	AROS_LH1(BOOL, IsCyberModeID,

/*  SYNOPSIS */
	AROS_LHA(ULONG, modeID, D0),

/*  LOCATION */
	struct Library *, CyberGfxBase, 9, Cybergraphics)

/*  FUNCTION
	Check if the given display mode ID belongs to an RTG driver

    INPUTS
	modeID - a display mode ID to check

    RESULT
        TRUE if the mode belongs to an RTG driver, FALSE otherwise

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

    /* Check if the given mode really exists and if it belongs to RTG.
       TODO: remove hardcoded value, use anything else */
    if (FindDisplayInfo(modeID) && modeID >= 0x00100000)
        return TRUE;

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* IsCyberModeID */
