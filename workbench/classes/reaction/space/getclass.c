/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction space - GetClass entry point
*/

#include <intuition/classes.h>
#include "space_intern.h"

/*****************************************************************************

    NAME */
#include <proto/space.h>

        AROS_LH0(Class *, SPACE_GetClass,

/*  LOCATION */
        struct Library *, SpaceBase, 5, Space)

/*  FUNCTION
        Returns a pointer to the space BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(SpaceBase);

    AROS_LIBFUNC_EXIT
}
