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

	AROS_LH7(ULONG, MovePixelArray,

/*  SYNOPSIS */
	AROS_LHA(UWORD            , SrcX, D0),
	AROS_LHA(UWORD            , SrcY, D1),
	AROS_LHA(struct RastPort *, RastPort, A1),
	AROS_LHA(UWORD            , DstX, D2),
	AROS_LHA(UWORD            , DstY, D3),
	AROS_LHA(UWORD            , SizeX, D4),
	AROS_LHA(UWORD            , SizeY, D5),

/*  LOCATION */
	struct Library *, CyberGfxBase, 22, Cybergraphics)

/*  FUNCTION
        Copies the pixels in a rectangular portion of a RastPort to another
        rectangle with the same dimensions in the same RastPort.

    INPUTS
        SrcX, SrcY - top-lefthand corner of source rectangle.
        RastPort - the RastPort to modify.
        DstX, DstY - top-lefthand corner of destination rectangle.
        SizeX, SizeY - size of the rectangles (in pixels).

    RESULT
        count - the number of pixels moved.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    return driver_MovePixelArray(SrcX, SrcY, RastPort, DstX, DstY, SizeX, SizeY, GetCGFXBase(CyberGfxBase));

    AROS_LIBFUNC_EXIT
} /* MovePixelArray */
