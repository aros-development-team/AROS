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

	AROS_LH4(LONG, WriteRGBPixel,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp		, A1),
	AROS_LHA(UWORD            , x		, D0),
	AROS_LHA(UWORD            , y		, D1),
	AROS_LHA(ULONG            , pixel	, D2),

/*  LOCATION */
	struct Library *, CyberGfxBase, 19, Cybergraphics)

/*  FUNCTION
        Writes a new color value to a pixel in a RastPort.

    INPUTS
        rp - the RastPort to write to.
        x, y - the coordinates of the pixel to write.
        pixel - the pixel's new color value in 32-bit ARGB format: 1 byte
            per component, in the order alpha, red, green, blue.

    RESULT
        error - 0 if no error occurred, or -1 if (x, y) is outside the
            RastPort.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    return driver_WriteRGBPixel(rp, x, y, pixel, GetCGFXBase(CyberGfxBase));

    AROS_LIBFUNC_EXIT
} /* WriteRGBPixel */
