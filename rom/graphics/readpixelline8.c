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

	LONG ReadPixelLine8 (

/*  SYNOPSIS */
	struct RastPort * rp,
	ULONG             xstart,
	ULONG             ystart,
	ULONG             width,
	UBYTE           * array,
	struct RastPort * tempRP)

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
    
    return driver_ReadPixelLine8(rp
    	, xstart, ystart
	, width
	, array, tempRP
	, GfxBase
    );

    AROS_LIBFUNC_EXIT
} /* ReadPixelLine8 */
