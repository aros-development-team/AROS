/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function FreeSpriteData()
    Lang: english
*/
#include <aros/debug.h>
#include <graphics/sprite.h>

#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH1(void, FreeSpriteData,

/*  SYNOPSIS */
        AROS_LHA(struct ExtSprite *, extsp, A2),

/*  LOCATION */
        struct GfxBase *, GfxBase, 172, Graphics)

/*  FUNCTION
	Free sprite data allocated by AllocSpriteData().

    INPUTS
        extsp - The extended sprite structure to be freed.
                Passing NULL is a NO-OP.

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

    if (NULL != extsp) {
        if (NULL != extsp->es_BitMap) {
            FreeBitMap(extsp->es_BitMap);
        }
        FreeVec(extsp);
    }
    
    AROS_LIBFUNC_EXIT
} /* FreeSpriteData */
