/*
    Copyright (C) 1995-2007, The AROS Development Team. All rights reserved.

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

        AROS_LH3(LONG, SetProtectionRelative,

/*  SYNOPSIS */
        AROS_LHA(BPTR,         lock,    D1),
        AROS_LHA(CONST_STRPTR, name,    D2),
        AROS_LHA(ULONG,        protect, D3),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 236, Dos)

/*  FUNCTION

    INPUTS
        name    - name of the file
        protect - new protection bits

    RESULT
        != 0 if all went well, 0 else. IoErr() gives additional
        information in that case.

    NOTES
        This call is AROS-specific.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG status = 0;
    struct PacketHelperStruct phs;

    D(bug("[SetProtectionRelative] lock=0x%p '%s':%x\n", lock, name, protect));

    if (getpacketinfo(DOSBase, lock, name, &phs)) {
        status = dopacket4(DOSBase, NULL, phs.port, ACTION_SET_PROTECT, BNULL, phs.lock, phs.name, protect);
        freepacketinfo(DOSBase, &phs);
    }

    return status;

    AROS_LIBFUNC_EXIT
} /* SetProtectionRelative */
