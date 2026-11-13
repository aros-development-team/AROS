/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Set the protection bits of a file.
*/
#include <aros/debug.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
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

    D(bug("[SetProtection] '%s':%x\n", name, protect));

    /* Multi-user: LIMITDOSSETPROTECTION may keep the GROUP/OTHER bits */
    if (SECURITY_ACTIVE)
        protect = secLimitProtection(name, protect);

    return pathpacket(DOSBase, name, ACTION_SET_PROTECT, TRUE, protect, 0);

    AROS_LIBFUNC_EXIT
} /* SetProtection */
