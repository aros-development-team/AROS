/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$    $Log
    Desc:
    Lang: english
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
	#include <graphics/rastport.h>
	#include <clib/graphics_protos.h>

	__AROS_LH1(ULONG, GetAPen,

/*  SYNOPSIS */
	__AROS_LHA(struct RastPort *, rp, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 143, Graphics)

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

    return rp->FgPen;

    __AROS_FUNC_EXIT
} /* GetAPen */
