/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$    $Log
    Desc:
    Lang: english
*/
#include "graphics_intern.h"

void driver_PolyDraw (struct RastPort *, long, WORD *, struct GfxBase *);

/*****************************************************************************

    NAME */
	#include <clib/graphics_protos.h>

	__AROS_LH3(void, PolyDraw,

/*  SYNOPSIS */
	__AROS_LHA(struct RastPort *, rp, A1),
	__AROS_LHA(long             , count, D0),
	__AROS_LHA(WORD            *, polyTable, A0),

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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct GfxBase *,GfxBase)

    driver_PolyDraw (rp, count, polyTable, GfxBase);

    __AROS_FUNC_EXIT
} /* PolyDraw */
