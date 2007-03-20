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

	AROS_LH6(LONG, WritePixelLine8,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *	, rp		, A0),
	AROS_LHA(LONG			, xstart	, D0),
	AROS_LHA(LONG			, ystart	, D1),
	AROS_LHA(ULONG			, width		, D2),
	AROS_LHA(UBYTE *		, array		, A2),
	AROS_LHA(struct RastPort * 	, tempRP	, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 129, Graphics)

/*  FUNCTION
	Draw a horizontal line from an array of pens.

    INPUTS
	rp            - destination RastPort
	xstart,ystart - start coordinate of line
	width         - count of pixels to write (must be positive)
	array         - array of pen values. Allocate at least
	                ((width+15)>>4)<<4 bytes.
	tempRP        - temporary rastport
	                (copy of rp with Layer set to NULL,
	                temporary memory allocated for
	                temprp->BitMap with Rows set to 1,
	                temprp->BitMap BytesPerRow == ((width+15)>>4)<<1,
	                and temporary memory allocated for
	                temprp->BitMap->Planes[])

    RESULT
	Number of plotted pixels.

    NOTES

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

    return WritePixelArray8(rp,
    	    	    	    xstart,
			    ystart,
			    xstart + width - 1,
			    ystart,
			    array,
			    tempRP);
    
    AROS_LIBFUNC_EXIT
    
} /* WritePixelLine8 */
