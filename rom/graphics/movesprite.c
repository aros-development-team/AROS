/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function MoveSprite()
    Lang: english
*/
#include <aros/debug.h>
#include <graphics/view.h>
#include <graphics/sprite.h>
#include <oop/oop.h>

#include "graphics_intern.h"

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
	Move sprite to a new position on the screen. Coordinates
	are specified relatively to given ViewPort, or relatively
	to the entire View (physical display) if the ViewPort is NULL.

	This function works also with extended sprites, since
	struct SimpleSprite is a part of struct ExtSprite.

    INPUTS
	vp     - a ViewPort for relative sprite positioning or NULL
	sprite - a pointer to a sprite descriptor structure
	x      - a new X coordinate
	y      - a new Y coordinate

    RESULT
	None.

    NOTES
	AROS currently supports only one sprite #0 for mouse pointer.
	Other sprite numbers are ignored by this function.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (vp) {
        sprite->x = x + vp->DxOffset;
        sprite->y = y + vp->DyOffset;
    } else {
        sprite->x = x;
	sprite->y = y;
    }

    if (sprite->num) /* We have only sprite #0 for the mouse cursor */
        return;

    if (SDD(GfxBase)->gfxhidd) {
    	HIDD_Gfx_SetCursorPos(SDD(GfxBase)->gfxhidd, sprite->x, sprite->y);
    }

    AROS_LIBFUNC_EXIT
} /* MoveSprite */
