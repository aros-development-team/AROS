/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction getfont - GetClass entry point
*/

#include <intuition/classes.h>
#include "getfont_intern.h"

/*****************************************************************************

    NAME */
#include <proto/getfont.h>

        AROS_LH0(Class *, GETFONT_GetClass,

/*  LOCATION */
        struct Library *, GetFontBase, 5, GetFont)

/*  FUNCTION
        Returns a pointer to the getfont BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(GetFontBase);

    AROS_LIBFUNC_EXIT
}
