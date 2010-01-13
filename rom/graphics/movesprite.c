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

    sprite->x = x;
    sprite->y = y;
    
    if (sprite->num) /* We have only sprite #0 for the mouse cursor */
        return;
	
    /* Note that we actually do not have ViewPorts and views, so we simply
       ignore the ViewPort */
    if (SDD(GfxBase)->gfxhidd)
    	HIDD_Gfx_SetCursorPos(SDD(GfxBase)->gfxhidd, x, y);

    AROS_LIBFUNC_EXIT
} /* MoveSprite */
