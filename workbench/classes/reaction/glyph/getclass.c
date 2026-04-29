/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction glyph - GetClass entry point
*/

#include <intuition/classes.h>
#include "glyph_intern.h"

/*****************************************************************************

    NAME */
#include <proto/glyph.h>

        AROS_LH0(Class *, GLYPH_GetClass,

/*  LOCATION */
        struct Library *, GlyphBase, 5, Glyph)

/*  FUNCTION
        Returns a pointer to the glyph BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(GlyphBase);

    AROS_LIBFUNC_EXIT
}
