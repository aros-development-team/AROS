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

	__AROS_LH7(void, ScrollRaster,

/*  SYNOPSIS */
	__AROS_LHA(struct RastPort *, rp, A1),
	__AROS_LHA(long             , dx, D0),
	__AROS_LHA(long             , dy, D1),
	__AROS_LHA(long             , xMin, D2),
	__AROS_LHA(long             , yMin, D3),
	__AROS_LHA(long             , xMax, D4),
	__AROS_LHA(long             , yMax, D5),

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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct GfxBase *,GfxBase)

    driver_ScrollRaster (rp, dx, dy, xMin, yMin, xMax, yMax, GfxBase);

    __AROS_FUNC_EXIT
} /* ScrollRaster */
