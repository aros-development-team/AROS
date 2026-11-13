/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction speedbar.gadget - GetClass entry point
*/

#include <intuition/classes.h>
#include "speedbar_intern.h"

/*****************************************************************************

    NAME */
#include <proto/speedbar.h>

        AROS_LH0(Class *, SPEEDBAR_GetClass,

/*  LOCATION */
        struct Library *, SpeedBarBase, 5, SpeedBar)

/*  FUNCTION
        Returns a pointer to the speedbar BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(SpeedBarBase);

    AROS_LIBFUNC_EXIT
}
