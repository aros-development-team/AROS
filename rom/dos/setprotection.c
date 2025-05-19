/*
    Copyright (C) 1995-2007, The AROS Development Team. All rights reserved.

    Desc: Set the protection bits of a file.
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(LONG, SetProtection,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name,    D1),
        AROS_LHA(ULONG,  protect, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 31, Dos)

/*  FUNCTION

    INPUTS
        name    - name of the file
        protect - new protection bits

    RESULT
        != 0 if all went well, 0 else. IoErr() gives additional
        information in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return SetProtectionRelative(BNULL, name, protect);

    AROS_LIBFUNC_EXIT
} /* SetProtection */
