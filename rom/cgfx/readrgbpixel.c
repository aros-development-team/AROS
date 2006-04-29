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

	AROS_LH3(ULONG, ReadRGBPixel,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp	, A1),
	AROS_LHA(UWORD            , x	, D0),
	AROS_LHA(UWORD            , y	, D1),

/*  LOCATION */
	struct Library *, CyberGfxBase, 18, Cybergraphics)

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
    
    return driver_ReadRGBPixel(rp, x, y, GfxBase);

    AROS_LIBFUNC_EXIT
} /* ReadRGBPixel */
