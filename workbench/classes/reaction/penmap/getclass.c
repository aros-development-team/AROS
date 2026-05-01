/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction penmap - GetClass entry point
*/

#include <intuition/classes.h>
#include "penmap_intern.h"

/*****************************************************************************

    NAME */
#include <proto/penmap.h>

        AROS_LH0(Class *, PENMAP_GetClass,

/*  LOCATION */
        struct Library *, PenMapBase, 5, PenMap)

/*  FUNCTION
        Returns a pointer to the penmap BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(PenMapBase);

    AROS_LIBFUNC_EXIT
}
