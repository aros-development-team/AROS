/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/08/12 14:28:29  digulla
    Change forground color of Rastport

    Desc:
    Lang: english
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/graphics_protos.h>

	__AROS_LH2(void, SetAPen,

/*  SYNOPSIS */
	__AROS_LA(struct RastPort *, rp, A1),
	__AROS_LA(unsigned long    , pen, D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 57, Graphics)

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

    rp->FgPen = pen;

    __AROS_FUNC_EXIT
} /* SetAPen */
