/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction scroller - GetClass entry point
*/

#include <intuition/classes.h>
#include "scroller_intern.h"

/*****************************************************************************

    NAME */
#include <proto/scroller.h>

        AROS_LH0(Class *, SCROLLER_GetClass,

/*  LOCATION */
        struct Library *, ScrollerBase, 5, Scroller)

/*  FUNCTION
        Returns a pointer to the scroller BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(ScrollerBase);

    AROS_LIBFUNC_EXIT
}
