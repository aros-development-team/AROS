/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function MoveSprite()
    Lang: english
*/
#include <aros/debug.h>
#include <graphics/view.h>
#include <graphics/sprite.h>

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH4(void, MoveSprite,

/*  SYNOPSIS */
        AROS_LHA(struct ViewPort *, vp, A0),
        AROS_LHA(struct SimpleSprite *, sprite, A1),
        AROS_LHA(WORD, x, D0),
        AROS_LHA(WORD, y, D1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 71, Graphics)

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
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

#warning TODO: Write graphics/MoveSprite()
    aros_print_not_implemented ("MoveSprite");

    AROS_LIBFUNC_EXIT
} /* MoveSprite */
