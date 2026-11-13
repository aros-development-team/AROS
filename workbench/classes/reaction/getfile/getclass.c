/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction getfile - GetClass entry point
*/

#include <intuition/classes.h>
#include "getfile_intern.h"

/*****************************************************************************

    NAME */
#include <proto/getfile.h>

        AROS_LH0(Class *, GETFILE_GetClass,

/*  LOCATION */
        struct Library *, GetFileBase, 5, GetFile)

/*  FUNCTION
        Returns a pointer to the getfile BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(GetFileBase);

    AROS_LIBFUNC_EXIT
}
