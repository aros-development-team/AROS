/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <hidd/gfx.h>
#include <aros/debug.h>

#include "cybergraphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/cybergraphics.h>

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

    ClipBlit(RastPort, SrcX, SrcY, RastPort, DstX, DstY, SizeX, SizeY,
        0x00C0); /* Copy */

    return SizeX * SizeY;

    AROS_LIBFUNC_EXIT
} /* MovePixelArray */
