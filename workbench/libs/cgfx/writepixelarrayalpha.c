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

	AROS_LH10(ULONG, WritePixelArrayAlpha,

/*  SYNOPSIS */
	AROS_LHA(APTR             , src		, A0),
	AROS_LHA(UWORD            , srcx	, D0),
	AROS_LHA(UWORD            , srcy	, D1),
	AROS_LHA(UWORD            , srcmod	, D2),
	AROS_LHA(struct RastPort *, rp		, A1),
	AROS_LHA(UWORD            , destx	, D3),
	AROS_LHA(UWORD            , desty	, D4),
	AROS_LHA(UWORD            , width	, D5),
	AROS_LHA(UWORD            , height	, D6),
	AROS_LHA(ULONG            , globalalpha	, D7),

/*  LOCATION */
	struct Library *, CyberGfxBase, 36, Cybergraphics)

/*  FUNCTION
        Alpha-blends all or part of a rectangular block of raw pixel values
        into a RastPort. The source data must be in 32-bit ARGB format: 1 byte
        per component, in the order alpha, red, green, blue.

    INPUTS
        srcRect - pointer to the pixel values.
        srcx, srcy - top-lefthand corner of portion of source rectangle to
            use (in pixels).
        srcmod - the number of bytes in each row of the source rectangle.
        rp - the RastPort to write to.
        destx, desty - top-lefthand corner of portion of destination RastPort
            to write to (in pixels).
        width, height - size of the affected area (in pixels).
        globalalpha - an alpha value applied globally to every pixel taken
            from the source rectangle (the full 32-bit range of values is
            used: 0 to 0xFFFFFFFF).

    RESULT
        count - the number of pixels written to.

    NOTES

    EXAMPLE

    BUGS
        The globalalpha parameter is currently ignored.

    SEE ALSO
        WritePixelArray()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    if (width && height)
    {
	return driver_WritePixelArrayAlpha(src
    	    , srcx, srcy
	    , srcmod
	    , rp
	    , destx, desty
	    , width, height
	    , globalalpha
	    , GetCGFXBase(CyberGfxBase)
	);
    }
    else return 0;

    AROS_LIBFUNC_EXIT
} /* WritePixelArrayAlpha */
