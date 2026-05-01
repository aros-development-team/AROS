/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction bitmap - GetClass entry point
*/

#include <intuition/classes.h>
#include "bitmap_intern.h"

/*****************************************************************************

    NAME */
#include <proto/bitmap.h>

        AROS_LH0(Class *, BITMAP_GetClass,

/*  LOCATION */
        struct Library *, BitMapBase, 5, BitMap)

/*  FUNCTION
        Returns a pointer to the bitmap BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(BitMapBase);

    AROS_LIBFUNC_EXIT
}
