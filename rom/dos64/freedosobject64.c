/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Free 64-bit dos objects.
*/

#include <proto/exec.h>
#include "dos64_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos64.h>

        AROS_LH2(void, FreeDosObject64,

/*  SYNOPSIS */
        AROS_LHA(ULONG, type, D1),
        AROS_LHA(APTR,  ptr,  D2),

/*  LOCATION */
        struct Dos64Base *, DOS64Base, 22, Dos64)

/*  FUNCTION
        Frees an object allocated with AllocDosObject64().

    INPUTS
        type - object type the object was allocated with
        ptr  - pointer to the object; may be NULL

    RESULT
        None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        AllocDosObject64()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    switch (type)
    {
    case DOS64_FIB:
    case DOS64_INFODATA:
    case DOS64_RECORDLOCK:
        FreeVec(ptr);
        break;
    default:
        break;
    }

    AROS_LIBFUNC_EXIT
} /* FreeDosObject64 */
