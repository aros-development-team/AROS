/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$    $Log
    Desc:
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/rastport.h>

extern void driver_ScrollRaster (struct RastPort *,
	long, long, long, long, long, long,
	struct GfxBase *);

/*****************************************************************************

    NAME */
	#include <graphics/rastport.h>
	#include <clib/graphics_protos.h>

	AROS_LH7(void, ScrollRaster,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(long             , dx, D0),
	AROS_LHA(long             , dy, D1),
	AROS_LHA(long             , xMin, D2),
	AROS_LHA(long             , yMin, D3),
	AROS_LHA(long             , xMax, D4),
	AROS_LHA(long             , yMax, D5),

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
