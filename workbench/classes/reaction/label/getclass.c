/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction label - GetClass entry point
*/

#include <intuition/classes.h>
#include "label_intern.h"

/*****************************************************************************

    NAME */
#include <proto/label.h>

        AROS_LH0(Class *, LABEL_GetClass,

/*  LOCATION */
        struct Library *, LabelBase, 5, Label)

/*  FUNCTION
        Returns a pointer to the label BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(LabelBase);

    AROS_LIBFUNC_EXIT
}
