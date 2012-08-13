/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function WaitBOVP()
    Lang: english
*/
#include <aros/debug.h>
#include <graphics/view.h>

#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH1(void, WaitBOVP,

/*  SYNOPSIS */
	AROS_LHA(struct ViewPort *, vp, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 67, Graphics)

/*  FUNCTION

    INPUTS
	vp - pointer to ViewPort structure

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

    /* TODO: Write graphics/WaitBOVP() */
    aros_print_not_implemented ("WaitBOVP");

    AROS_LIBFUNC_EXIT
} /* WaitBOVP */
