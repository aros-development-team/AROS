/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function CalcIVG()
    Lang: english
*/
#include <aros/debug.h>
#include <graphics/view.h>

#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH2(UWORD, CalcIVG,

/*  SYNOPSIS */
        AROS_LHA(struct View *, View, A0),
        AROS_LHA(struct ViewPort *, ViewPort, A1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 138, Graphics)

/*  FUNCTION
	Calculate the number of blank lines above a ViewPort.

    INPUTS
        View     - pointer to the View
        ViewPort - pointer to the ViewPort you are interested in

    RESULT
        count - the number of ViewPort resolution scan lines needed
                to execute all the copper instructions for ViewPort,
                or 0 if any error

    NOTES

    EXAMPLE

    BUGS
        This function is unimplemented.

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* TODO: Write graphics/CalcIVG() */
    aros_print_not_implemented ("CalcIVG");

    return 0;

    AROS_LIBFUNC_EXIT
} /* CalcIVG */
