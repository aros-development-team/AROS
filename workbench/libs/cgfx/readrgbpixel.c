/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
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
        Reads a particular pixel's color value from a RastPort.

    INPUTS
        rp - the RastPort to read from.
        x, y - the coordinates of the pixel to read.

    RESULT
        color - the pixel's color value in 32-bit ARGB format: 1 byte
            per component, in the order alpha, red, green, blue.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    return driver_ReadRGBPixel(rp, x, y, GetCGFXBase(CyberGfxBase));

    AROS_LIBFUNC_EXIT
} /* ReadRGBPixel */
