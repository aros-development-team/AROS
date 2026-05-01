/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction bevel - GetClass entry point
*/

#include <intuition/classes.h>
#include "bevel_intern.h"

/*****************************************************************************

    NAME */
#include <proto/bevel.h>

        AROS_LH0(Class *, BEVEL_GetClass,

/*  LOCATION */
        struct Library *, BevelBase, 5, Bevel)

/*  FUNCTION
        Returns a pointer to the bevel BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(BevelBase);

    AROS_LIBFUNC_EXIT
}
