/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "cybergraphics_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cybergraphics_protos.h>

	AROS_LH2(ULONG, GetCyberMapAttr,

/*  SYNOPSIS */
	AROS_LHA(struct BitMap *, bitMap, A0),
	AROS_LHA(ULONG          , attribute, D0),

/*  LOCATION */
	struct Library *, CyberGfxBase, 16, Cybergraphics)

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
    
    return driver_GetCyberMapAttr(bitMap, attribute, GetCGFXBase(CyberGfxBase));

    AROS_LIBFUNC_EXIT
} /* GetCyberMapAttr */
