/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function MakeVPort()
    Lang: english
*/
#include <aros/debug.h>
#include <graphics/view.h>

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH2(ULONG, MakeVPort,

/*  SYNOPSIS */
        AROS_LHA(struct View *, view, A0),
        AROS_LHA(struct ViewPort *, viewport, A1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 36, Graphics)

/*  FUNCTION

    INPUTS
        view     - pointer to a View structure
        viewport - pointer to a ViewPort structure
                   the viewport must have a valid pointer to a RasInfo

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

#warning TODO: Write graphics/MakeVPort()
    aros_print_not_implemented ("MakeVPort");

    return MVP_NO_MEM;

    AROS_LIBFUNC_EXIT
} /* MakeVPort */
