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

	AROS_LH5(ULONG, InvertPixelArray,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp		, A1),
	AROS_LHA(UWORD            , destx	, D0),
	AROS_LHA(UWORD            , desty	, D1),
	AROS_LHA(UWORD            , width	, D2),
	AROS_LHA(UWORD            , height	, D3),

/*  LOCATION */
	struct Library *, CyberGfxBase, 24, Cybergraphics)

/*  FUNCTION
        Inverts each pixel in rectangular portion of a RastPort, i.e. applies
        a NOT operation to each bit of pixel data.

    INPUTS
        rp - the RastPort to write to.
        destx, desty - top-lefthand corner of portion of RastPort to invert.
        width, height - size of the area to invert.

    RESULT
        count - the number of pixels inverted.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    return driver_InvertPixelArray(rp
    	, destx, desty
	, width, height
	, GetCGFXBase(CyberGfxBase)
    );

    AROS_LIBFUNC_EXIT
} /* InvertPixelArray */
