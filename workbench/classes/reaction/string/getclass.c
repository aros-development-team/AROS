/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction string - GetClass entry point
*/

#include <intuition/classes.h>
#include "string_intern.h"

/*****************************************************************************

    NAME */
#include <proto/string.h>

        AROS_LH0(Class *, STRING_GetClass,

/*  LOCATION */
        struct Library *, StringBase, 5, String)

/*  FUNCTION
        Returns a pointer to the string BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(StringBase);

    AROS_LIBFUNC_EXIT
}
