/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "cybergraphics_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cybergraphics_protos.h>

	AROS_LH4(LONG, WriteRGBPixel,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp		, A1),
	AROS_LHA(UWORD            , x		, D0),
	AROS_LHA(UWORD            , y		, D1),
	AROS_LHA(ULONG            , pixel	, D2),

/*  LOCATION */
	struct Library *, CyberGfxBase, 19, Cybergraphics)

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
    AROS_LIBBASE_EXT_DECL(struct Library *,CyberGfxBase)
    
    return driver_WriteRGBPixel(rp, x, y, pixel, GfxBase);

    AROS_LIBFUNC_EXIT
} /* WriteRGBPixel */
