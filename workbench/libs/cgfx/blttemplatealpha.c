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

	AROS_LH8(void, BltTemplateAlpha,

/*  SYNOPSIS */
	AROS_LHA(APTR             , src		, A0),
	AROS_LHA(LONG             , srcx	, D0),
	AROS_LHA(LONG             , srcmod	, D1),
	AROS_LHA(struct RastPort *, rp		, A1),
	AROS_LHA(LONG             , destx	, D2),
	AROS_LHA(LONG             , desty	, D3),
	AROS_LHA(LONG	          , width	, D4),
	AROS_LHA(LONG             , height	, D5),

/*  LOCATION */
	struct Library *, CyberGfxBase, 37, Cybergraphics)

/*  FUNCTION
        Alpha blends the current foreground colour into a rectangular portion
        of a RastPort. The source alpha channel to use for each pixel is taken
        from an array of 8-bit alpha values. This alpha template may be any
        rectangle within a larger array/rectangle of alpha values.

    INPUTS
        src - pointer to an array of source alpha values.
        srcx - byte/pixel offset of top-lefthand corner of alpha template.
        srcmod - the number of bytes in each row of the source array.
        rp - the RastPort to write to.
        destx, desty - top-lefthand corner of portion of destination RastPort
            to write to (in pixels).
        width, height - size of the area to copy (in pixels).

    RESULT
        None.

    NOTES
        The size and destination coordinates may be outside the RastPort
        boundaries, in which case the affected area is safely truncated.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    if (width && height)
    {
	driver_BltTemplateAlpha(src
    	    , srcx
	    , srcmod
	    , rp
	    , destx, desty
	    , width, height
	    , GetCGFXBase(CyberGfxBase)
	);
    }

    AROS_LIBFUNC_EXIT
} /* BltTemplateAlpha */
