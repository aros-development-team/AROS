/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH7(LONG, ReadPixelArray8,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *	, rp		, A0),
	AROS_LHA(ULONG             	, xstart	, D0),
	AROS_LHA(ULONG             	, ystart	, D1),
	AROS_LHA(ULONG             	, xstop		, D2),
	AROS_LHA(ULONG             	, ystop		, D3),
	AROS_LHA(UBYTE * 		, array		, A2),
	AROS_LHA(struct RastPort *	, temprp	, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 130, Graphics)

/*  FUNCTION

    INPUTS

    RESULT

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

    return driver_ReadPixelArray8(rp
    		, xstart, ystart
		, xstop, ystop
		, array, temprp
		, GfxBase
    );

    AROS_LIBFUNC_EXIT
} /* ReadPixelArray8 */
