/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function MrgCop()
    Lang: english
*/
#include <aros/debug.h>
#include <graphics/view.h>

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH1(ULONG, MrgCop,

/*  SYNOPSIS */
        AROS_LHA(struct View *, view, A1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 35, Graphics)

/*  FUNCTION

    INPUTS
        view -

    RESULT
        error -

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

#warning TODO: Write graphics/MrgCop()
    aros_print_not_implemented ("MrgCop");

    return MCOP_NO_MEM;

    AROS_LIBFUNC_EXIT
} /* MrgCop */
