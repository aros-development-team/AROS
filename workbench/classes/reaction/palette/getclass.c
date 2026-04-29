/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction palette - GetClass entry point
*/

#include <intuition/classes.h>
#include "palette_intern.h"

/*****************************************************************************

    NAME */
#include <proto/palette.h>

        AROS_LH0(Class *, PALETTE_GetClass,

/*  LOCATION */
        struct Library *, PaletteBase, 5, Palette)

/*  FUNCTION
        Returns a pointer to the palette BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(PaletteBase);

    AROS_LIBFUNC_EXIT
}
