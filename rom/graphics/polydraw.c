/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$    $Log

    Desc: Graphics function PolyDraw()
    Lang: english
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/graphics_protos.h>

	AROS_LH3(void, PolyDraw,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(long             , count, D0),
	AROS_LHA(WORD            *, polyTable, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 56, Graphics)

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

    driver_PolyDraw (rp, count, polyTable, GfxBase);

    AROS_LIBFUNC_EXIT
} /* PolyDraw */
