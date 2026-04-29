/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction requester - GetClass entry point
*/

#include <intuition/classes.h>
#include "requester_intern.h"

/*****************************************************************************

    NAME */
#include <proto/requester.h>

        AROS_LH0(Class *, REQUESTER_GetClass,

/*  LOCATION */
        struct Library *, RequesterBase, 5, Requester)

/*  FUNCTION
        Returns a pointer to the requester BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(RequesterBase);

    AROS_LIBFUNC_EXIT
}
