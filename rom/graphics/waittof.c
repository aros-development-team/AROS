/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/graphics_protos.h>

	AROS_LH0(void, WaitTOF,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct GfxBase *, GfxBase, 45, Graphics)

/*  FUNCTION
	Wait for the video beam to return to the upper left edge. During
	this time, changes in the visible screens will not be noticed by
	the user (because the monitor doesn't display anything).

    INPUTS
	None.

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    driver_WaitTOF (GfxBase);

    AROS_LIBFUNC_EXIT
} /* WaitTOF */
