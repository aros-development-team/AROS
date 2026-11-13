/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction getscreenmode - GetClass entry point
*/

#include <intuition/classes.h>
#include "getscreenmode_intern.h"

/*****************************************************************************

    NAME */
#include <proto/getscreenmode.h>

        AROS_LH0(Class *, GETSCREENMODE_GetClass,

/*  LOCATION */
        struct Library *, GetScreenModeBase, 5, GetScreenMode)

/*  FUNCTION
        Returns a pointer to the getscreenmode BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(GetScreenModeBase);

    AROS_LIBFUNC_EXIT
}
