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

	AROS_LH7(void, WriteChunkyPixels,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A0),
	AROS_LHA(ULONG            , xstart, D0),
	AROS_LHA(ULONG            , ystart, D1),
	AROS_LHA(ULONG            , xstop, D2),
	AROS_LHA(ULONG            , ystop, D3),
	AROS_LHA(UBYTE           *, array, A2),
	AROS_LHA(LONG             , bytesperrow, D4),

/*  LOCATION */
	struct GfxBase *, GfxBase, 176, Graphics)

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
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)
    
    driver_WriteChunkyPixels(rp
    	, xstart, ystart
	, xstop, ystop
	, array, bytesperrow
	, GfxBase
    );

    AROS_LIBFUNC_EXIT
} /* WriteChunkyPixels */
