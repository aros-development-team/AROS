/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Create 64-bit dos objects.
*/

#include <exec/memory.h>
#include <proto/exec.h>
#include "dos64_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos64.h>

        AROS_LH2(APTR, AllocDosObject64,

/*  SYNOPSIS */
        AROS_LHA(ULONG,                  type, D1),
        AROS_LHA(const struct TagItem *, tags, D2),

/*  LOCATION */
        struct Dos64Base *, DOS64Base, 21, Dos64)

/*  FUNCTION
        Creates a dos object of a given type, for use with the 64-bit
        dos64.library functions. Free the object with FreeDosObject64()
        when done.

    INPUTS
        type - object type (DOS64_FIB, DOS64_INFODATA, DOS64_RECORDLOCK,
               see <dos/dos64.h>)
        tags - future extensions, pass NULL

    RESULT
        Pointer to the new object, or NULL on failure. IoErr() gives
        additional information in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        FreeDosObject64(), dos.library/AllocDosObject()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG size;

    switch (type)
    {
    case DOS64_FIB:
        size = sizeof(struct FileInfoBlock64);
        break;
    case DOS64_INFODATA:
        size = sizeof(struct InfoData64);
        break;
    case DOS64_RECORDLOCK:
        size = sizeof(struct RecordLock64);
        break;
    default:
        SetIoErr(ERROR_BAD_NUMBER);
        return NULL;
    }

    return AllocVec(size, MEMF_PUBLIC | MEMF_CLEAR);

    AROS_LIBFUNC_EXIT
} /* AllocDosObject64 */
