/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Set the owner of a file.
*/
#include <aros/debug.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
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

    D(bug("[SetOwner] '%s' %x\n", name, owner_info));

    return pathpacket(DOSBase, name, ACTION_SET_OWNER, TRUE, (SIPTR)owner_info, 0);

    AROS_LIBFUNC_EXIT
} /* SetOwner */
