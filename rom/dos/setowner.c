/*
    Copyright (C) 1995-2013, The AROS Development Team. All rights reserved.

    Desc: Set the owner of a file.
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(BOOL, SetOwner,

/*  SYNOPSIS */
        AROS_LHA(STRPTR, name,       D1),
        AROS_LHA(ULONG,  owner_info, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 166, Dos)

/*  FUNCTION

    INPUTS
        name       - name of the file
        owner_info - (UID << 16) + GID

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

    return SetOwnerRelative(BNULL, name, owner_info);

    AROS_LIBFUNC_EXIT
} /* SetOwner */
