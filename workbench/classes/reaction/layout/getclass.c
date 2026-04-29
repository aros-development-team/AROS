/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction layout.gadget - GetClass entry point
*/

#include <intuition/classes.h>
#include "layout_intern.h"

/*****************************************************************************

    NAME */
#include <proto/layout.h>

        AROS_LH0(Class *, LAYOUT_GetClass,

/*  LOCATION */
        struct Library *, LayoutBase, 5, Layout)

/*  FUNCTION
        Returns a pointer to the layout BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(LayoutBase);

    AROS_LIBFUNC_EXIT
}
