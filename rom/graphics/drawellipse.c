/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$    $Log

    Desc: Graphics function DrawEllipse
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/rastport.h>

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH5(void, DrawEllipse,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(LONG             , xCenter, D0),
	AROS_LHA(LONG             , yCenter, D1),
	AROS_LHA(LONG             , a, D2),
	AROS_LHA(LONG             , b, D3),

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
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    driver_DrawEllipse (rp, xCenter, yCenter, a, b, GfxBase);

    AROS_LIBFUNC_EXIT
} /* DrawEllipse */
