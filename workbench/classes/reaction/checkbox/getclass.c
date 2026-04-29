/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction checkbox - GetClass entry point
*/

#include <intuition/classes.h>
#include "checkbox_intern.h"

/*****************************************************************************

    NAME */
#include <proto/checkbox.h>

        AROS_LH0(Class *, CHECKBOX_GetClass,

/*  LOCATION */
        struct Library *, CheckBoxBase, 5, CheckBox)

/*  FUNCTION
        Returns a pointer to the checkbox BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(CheckBoxBase);

    AROS_LIBFUNC_EXIT
}
