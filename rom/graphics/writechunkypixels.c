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

	void WriteChunkyPixels (

/*  SYNOPSIS */
	struct RastPort * rp,
	ULONG             xstart,
	ULONG             ystart,
	ULONG             xstop,
	ULONG             ystop,
	UBYTE           * array,
	LONG              bytesperrow)

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
    
    driver_WriteChunkyPixels(rp
    	, xstart, ystart
	, xstop, ystop
	, array, bytesperrow
	, GfxBase
    );

    AROS_LIBFUNC_EXIT
} /* WriteChunkyPixels */
