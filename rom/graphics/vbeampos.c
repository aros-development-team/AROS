/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Read the current vertical position of the beam
    Lang: english
*/
#include <aros/debug.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/graphics_protos.h>

	AROS_LH0(LONG, VBeamPos,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct GfxBase *, GfxBase, 64, Graphics)

/*  FUNCTION

    INPUTS
        none

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS
        This function is unimplemented.

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT


    /* TODO: Write graphics/VBeamPos() */
    aros_print_not_implemented ("VBeamPos");

    return 10;

    AROS_LIBFUNC_EXIT
} /* VBeamPos */
