/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function GetExtSpriteA()
    Lang: english
*/
#include <aros/debug.h>
#include <graphics/sprite.h>
#include <utility/tagitem.h>

#include "graphics_intern.h"

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

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
    	AROS does not have complete sprite system, instead it has a (hacky)
	minimal implementation enough to drive mouse pointer as a single
	sprite #0. This assumes that this sprite is always allocated by the OS
	itself and can't be used by user applications. So we just return error.

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return -1;

    AROS_LIBFUNC_EXIT
} /* GetExtSpriteA */
