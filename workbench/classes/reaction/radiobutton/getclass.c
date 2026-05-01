/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction radiobutton.gadget - GetClass entry point
*/

#include <intuition/classes.h>
#include "radiobutton_intern.h"

/*****************************************************************************

    NAME */
#include <proto/radiobutton.h>

        AROS_LH0(Class *, RADIOBUTTON_GetClass,

/*  LOCATION */
        struct Library *, RadioButtonBase, 5, RadioButton)

/*  FUNCTION
        Returns a pointer to the radiobutton BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(RadioButtonBase);

    AROS_LIBFUNC_EXIT
}
