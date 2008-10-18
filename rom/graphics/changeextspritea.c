/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function ChangeExtSpriteA()
    Lang: english
*/
#include <aros/debug.h>
#include <graphics/view.h>
#include <graphics/sprite.h>
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH4(LONG, ChangeExtSpriteA,

/*  SYNOPSIS */
        AROS_LHA(struct ViewPort *, vp, A0),
        AROS_LHA(struct ExtSprite *, oldsprite, A1),
        AROS_LHA(struct ExtSprite *, newsprite, A2),
        AROS_LHA(struct TagItem *, tags, A3),

/*  LOCATION */
        struct GfxBase *, GfxBase, 171, Graphics)

/*  FUNCTION

    INPUTS
        vp        - pointer to ViewPort structure that this sprite is
                    relative to, or NULL if relative only top of View
        oldsprite - pointer to the old ExtSprite structure
        newsprite - pointer to the new ExtSprite structure
        tags      - pointer to taglist

    RESULT
        success - 0 if there was an error

    NOTES
	Not implemented.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

#warning TODO: Write graphics/ChangeExtSpriteA()
    aros_print_not_implemented ("ChangeExtSpriteA");

    return 0;

    AROS_LIBFUNC_EXIT
} /* ChangeExtSpriteA */
