/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function ChangeSprite()
    Lang: english
*/
#include <aros/debug.h>
#include <graphics/view.h>
#include <graphics/sprite.h>

#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH3(void, ChangeSprite,

/*  SYNOPSIS */
        AROS_LHA(struct ViewPort *, vp, A0),
        AROS_LHA(struct SimpleSprite *, s, A1),
        AROS_LHA(void *, newdata, A2),

/*  LOCATION */
        struct GfxBase *, GfxBase, 70, Graphics)

/*  FUNCTION

    INPUTS

    RESULT

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

    /* TODO: Write graphics/ChangeSprite() */
    aros_print_not_implemented ("ChangeSprite");

    AROS_LIBFUNC_EXIT
} /* ChangeSprite */
