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

	AROS_LH6(ULONG, FillPixelArray,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp		, A1),
	AROS_LHA(UWORD            , destx	, D0),
	AROS_LHA(UWORD            , desty	, D1),
	AROS_LHA(UWORD            , width	, D2),
	AROS_LHA(UWORD            , height	, D3),
	AROS_LHA(ULONG            , pixel	, D4),

/*  LOCATION */
	struct Library *, CyberGfxBase, 25, Cybergraphics)

/*  FUNCTION
        Writes the same color value to all pixels in a rectangular region of
        a RastPort.

    INPUTS
        rp - the RastPort to write to.
        destx, desty - top-lefthand corner of portion of destination RastPort
            to write to (in pixels).
        width, height - size of the affected area (in pixels).
        pixel - the color value to use, in 32-bit ARGB format: 1 byte per
            component, in the order alpha, red, green, blue.

    RESULT
        count - the number of pixels filled.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    return driver_FillPixelArray(rp
    	, destx, desty
	, width, height
	, pixel
	, GetCGFXBase(CyberGfxBase)
    );

    AROS_LIBFUNC_EXIT
} /* FillPixelArray */
