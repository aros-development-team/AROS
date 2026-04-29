/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction window - GetClass entry point
*/

#include <intuition/classes.h>
#include "window_intern.h"

/*****************************************************************************

    NAME */
#include <proto/window.h>

        AROS_LH0(Class *, WINDOW_GetClass,

/*  LOCATION */
        struct Library *, WindowBase, 5, Window)

/*  FUNCTION
        Returns a pointer to the window BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(WindowBase);

    AROS_LIBFUNC_EXIT
}
