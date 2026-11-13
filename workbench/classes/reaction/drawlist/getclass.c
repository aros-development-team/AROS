/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction drawlist - GetClass entry point
*/

#include <intuition/classes.h>
#include "drawlist_intern.h"

/*****************************************************************************

    NAME */
#include <proto/drawlist.h>

        AROS_LH0(Class *, DRAWLIST_GetClass,

/*  LOCATION */
        struct Library *, DrawListBase, 5, DrawList)

/*  FUNCTION
        Returns a pointer to the drawlist BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(DrawListBase);

    AROS_LIBFUNC_EXIT
}
