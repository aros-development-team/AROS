/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$    $Log
    Desc:
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/rastport.h>

void driver_Draw (struct RastPort *, long, long, struct GfxBase *);

/*****************************************************************************

    NAME */
	#include <graphics/rastport.h>
	#include <clib/graphics_protos.h>

	__AROS_LH3(void, Draw,

/*  SYNOPSIS */
	__AROS_LHA(struct RastPort *, rp, A1),
	__AROS_LHA(long             , x, D0),
	__AROS_LHA(long             , y, D1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 41, Graphics)

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

    driver_Draw (rp, x, y, GfxBase);

    rp->cp_x = x;
    rp->cp_y = y;

    __AROS_FUNC_EXIT
} /* Draw */
