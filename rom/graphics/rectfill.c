/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$    $Log
    Desc:
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/rastport.h>

extern void driver_RectFill (struct RastPort *, long, long, long, long);

/*****************************************************************************

    NAME */
	#include <graphics/rastport.h>
	#include <clib/graphics_protos.h>

	__AROS_LH5(void, RectFill,

/*  SYNOPSIS */
	__AROS_LHA(struct RastPort *, rp, A1),
	__AROS_LHA(long             , xMin, D0),
	__AROS_LHA(long             , yMin, D1),
	__AROS_LHA(long             , xMax, D2),
	__AROS_LHA(long             , yMax, D3),

/*  LOCATION */
	struct GfxBase *, GfxBase, 51, Graphics)

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

    driver_RectFill (rp, xMin, yMin, xMax, yMax);

    __AROS_FUNC_EXIT
} /* RectFill */
