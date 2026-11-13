/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction clicktab.gadget - GetClass entry point
*/

#include <intuition/classes.h>
#include "clicktab_intern.h"

/*****************************************************************************

    NAME */
#include <proto/clicktab.h>

        AROS_LH0(Class *, CLICKTAB_GetClass,

/*  LOCATION */
        struct Library *, ClickTabBase, 5, ClickTab)

/*  FUNCTION
        Returns a pointer to the clicktab BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(ClickTabBase);

    AROS_LIBFUNC_EXIT
}
