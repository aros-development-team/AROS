/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction textfield - GetClass entry point
*/

#include <intuition/classes.h>
#include "textfield_intern.h"

/*****************************************************************************

    NAME */
#include <proto/textfield.h>

        AROS_LH0(Class *, TEXTFIELD_GetClass,

/*  LOCATION */
        struct Library *, TextFieldBase, 5, TextField)

/*  FUNCTION
        Returns a pointer to the textfield BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(TextFieldBase);

    AROS_LIBFUNC_EXIT
}
