/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction integer - GetClass entry point
*/

#include <intuition/classes.h>
#include "integer_intern.h"

/*****************************************************************************

    NAME */
#include <proto/integer.h>

        AROS_LH0(Class *, INTEGER_GetClass,

/*  LOCATION */
        struct Library *, IntegerBase, 5, Integer)

/*  FUNCTION
        Returns a pointer to the integer BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(IntegerBase);

    AROS_LIBFUNC_EXIT
}
