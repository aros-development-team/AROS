/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$    $Log
    Desc:
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/rastport.h>

LONG driver_DrawEllipse (struct RastPort *, long x, long y, long rx, long ry,
			struct GfxBase *);

/*****************************************************************************

    NAME */
	#include <graphics/rastport.h>
	#include <clib/graphics_protos.h>

	__AROS_LH5(void, DrawEllipse,

/*  SYNOPSIS */
	__AROS_LHA(struct RastPort *, rp, A1),
	__AROS_LHA(long             , xCenter, D0),
	__AROS_LHA(long             , yCenter, D1),
	__AROS_LHA(long             , a, D2),
	__AROS_LHA(long             , b, D3),

/*  LOCATION */
	struct GfxBase *, GfxBase, 30, Graphics)

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

    driver_DrawEllipse (rp, xCenter, yCenter, a, b, GfxBase);

    __AROS_FUNC_EXIT
} /* DrawEllipse */
