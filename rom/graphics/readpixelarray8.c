/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	LONG ReadPixelArray8 (

/*  SYNOPSIS */
	struct RastPort * rp,
	ULONG             xstart,
	ULONG             ystart,
	ULONG             xstop,
	ULONG             ystop,
	UBYTE           * array,
	struct RastPort * temprp)

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
