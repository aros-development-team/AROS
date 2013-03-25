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

	AROS_LH10(LONG, ScalePixelArray,

/*  SYNOPSIS */
	AROS_LHA(APTR             , srcRect, A0),
	AROS_LHA(UWORD            , SrcW, D0),
	AROS_LHA(UWORD            , SrcH, D1),
	AROS_LHA(UWORD            , SrcMod, D2),
	AROS_LHA(struct RastPort *, RastPort, A1),
	AROS_LHA(UWORD            , DestX, D3),
	AROS_LHA(UWORD            , DestY, D4),
	AROS_LHA(UWORD            , DestW, D5),
	AROS_LHA(UWORD            , DestH, D6),
	AROS_LHA(UBYTE            , SrcFormat, D7),

/*  LOCATION */
	struct Library *, CyberGfxBase, 15, Cybergraphics)

/*  FUNCTION
        Fills all or part of a RastPort with a rectangular block of raw pixel
        values. The source pixels are scaled to fit the destination area, i.e.
        some pixels may be duplicated or dropped according to the need to
        stretch or compress the source block.

    INPUTS
        srcRect - pointer to the pixel values.
        SrcW, SrcH - width and height of the source rectangle (in pixels).
        SrcMod - the number of bytes in each row of the source rectangle.
        RastPort - the RastPort to write to.
        DestX, DestY - top-lefthand corner of portion of destination RastPort
            to write to (in pixels).
        DestW, DestH - size of the destination rectangle (in pixels).
        SrcFormat - the format of the source pixels. See WritePixelArray for
            possible values.

    RESULT
        count - the number of pixels written to.

    NOTES

    EXAMPLE

    BUGS
        This function is unimplemented.

    SEE ALSO
        WritePixelArray()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

#if 0
    extern void aros_print_not_implemented (char *);

    aros_print_not_implemented ("ScalePixelArray");
#endif

    return 0;
    
    AROS_LIBFUNC_EXIT
} /* ScalePixelArray */
