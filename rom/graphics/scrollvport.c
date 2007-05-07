/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function ScrollVPort()
    Lang: english
*/
#include <aros/debug.h>
#include <graphics/view.h>

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH1(void, ScrollVPort,

/*  SYNOPSIS */
        AROS_LHA(struct ViewPort *, vp, A0),

/*  LOCATION */
        struct GfxBase *, GfxBase, 98, Graphics)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

#warning TODO: Write graphics/ScrollVPort()
    /* aros_print_not_implemented ("ScrollVPort");
       Commented out because causes slowdown of Directory Opus
       which calls this function on every mouse move (why???) */

    AROS_LIBFUNC_EXIT
} /* ScrollVPort */
