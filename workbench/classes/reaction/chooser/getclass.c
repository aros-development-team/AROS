/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction chooser.gadget - GetClass entry point
*/

#include <intuition/classes.h>
#include "chooser_intern.h"

/*****************************************************************************

    NAME */
#include <proto/chooser.h>

        AROS_LH0(Class *, CHOOSER_GetClass,

/*  LOCATION */
        struct Library *, ChooserBase, 5, Chooser)

/*  FUNCTION
        Returns a pointer to the chooser BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(ChooserBase);

    AROS_LIBFUNC_EXIT
}
