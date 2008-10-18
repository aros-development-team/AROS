/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function GetExtSpriteA()
    Lang: english
*/
#include <aros/debug.h>
#include <graphics/sprite.h>
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH2(LONG, GetExtSpriteA,

/*  SYNOPSIS */
        AROS_LHA(struct ExtSprite *, sprite, A2),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 155, Graphics)

/*  FUNCTION

    INPUTS
        sprite - pointer to programmer's ExtSprite (from AllocSpriteData())
        tags   - a standard tag list

    RESULT
        Sprite_number - a sprite number or -1 for an error

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

#warning TODO: Write graphics/GetExtSpriteA()
    aros_print_not_implemented ("GetExtSpriteA");

    return -1;

    AROS_LIBFUNC_EXIT
} /* GetExtSpriteA */
