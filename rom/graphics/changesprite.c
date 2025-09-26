/*
    Copyright (C) 1995-2007, The AROS Development Team. All rights reserved.

    Desc: Graphics function ChangeSprite()
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

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

    WORD x,y;

    s->posctldata = newdata;
    x = s->x;
    y = s->y;

    MoveSprite(vp, s, x, y);

    AROS_LIBFUNC_EXIT
} /* ChangeSprite */
