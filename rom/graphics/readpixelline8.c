/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH6(LONG, ReadPixelLine8,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort * 	, rp		, A0),
	AROS_LHA(LONG			, xstart	, D0),
	AROS_LHA(LONG			, ystart	, D1),
	AROS_LHA(ULONG			, width		, D2),
	AROS_LHA(UBYTE           *	, array		, A2),
	AROS_LHA(struct RastPort * 	, tempRP	, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 128, Graphics)

/*  FUNCTION
	Read the pen numbers of a horizontal line into an array.

    INPUTS
	rp            - RastPort
	xstart,ystart - coordinate
	width         - count of pixels to read (must be positive).
	array         - array for storing of the pen numbers. Size must be
	                at least ((width+15)>>4)<<4 bytes.
	tempRP        - see ReadPixelArray8().

    RESULT
	Number of pixels read.

    NOTES
	This function doesn't make sense on true-/hicolor rastports.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return ReadPixelArray8(rp,
    	    	    	   xstart,
			   ystart,
			   xstart + width - 1,
			   ystart,
			   array,
			   tempRP);

    AROS_LIBFUNC_EXIT
    
} /* ReadPixelLine8 */
