/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$	$Log

    Desc: Graphics function ScrollRaster()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/rastport.h>

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <clib/graphics_protos.h>

	AROS_LH7(void, ScrollRaster,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(LONG             , dx, D0),
	AROS_LHA(LONG             , dy, D1),
	AROS_LHA(LONG             , xMin, D2),
	AROS_LHA(LONG             , yMin, D3),
	AROS_LHA(LONG             , xMax, D4),
	AROS_LHA(LONG             , yMax, D5),

/*  LOCATION */
	struct GfxBase *, GfxBase, 66, Graphics)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    driver_ScrollRaster (rp, dx, dy, xMin, yMin, xMax, yMax, GfxBase);

    AROS_LIBFUNC_EXIT
} /* ScrollRaster */
