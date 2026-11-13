/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction button - GetClass entry point
*/

#include <intuition/classes.h>
#include "button_intern.h"

/*****************************************************************************

    NAME */
#include <proto/button.h>

        AROS_LH0(Class *, BUTTON_GetClass,

/*  LOCATION */
        struct Library *, ButtonBase, 5, Button)

/*  FUNCTION
        Returns a pointer to the button BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(ButtonBase);

    AROS_LIBFUNC_EXIT
}
