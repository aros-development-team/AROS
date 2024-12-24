/*
    Copyright (C) 1995-2013, The AROS Development Team. All rights reserved.

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

        AROS_LH3(BOOL, SetOwnerRelative,

/*  SYNOPSIS */
        AROS_LHA(BPTR,   lock,       D1),
        AROS_LHA(STRPTR, name,       D2),
        AROS_LHA(ULONG,  owner_info, D3),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 235, Dos)

/*  FUNCTION

    INPUTS
        name       - name of the file
        owner_info - (UID << 16) + GID

    RESULT
        != 0 if all went well, 0 else. IoErr() gives additional
        information in that case.

    NOTES
        This call is AROS-specific.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PacketHelperStruct phs;
    LONG status = DOSFALSE;

    D(bug("[SetOwnerRelative] lock=0x%p '%s' %x\n", lock, name, owner_info));

    if (getpacketinfo(DOSBase, lock, name, &phs)) {
        status = dopacket4(DOSBase, NULL, phs.port, ACTION_SET_OWNER, (IPTR)NULL, phs.lock, phs.name, (IPTR)owner_info);
        freepacketinfo(DOSBase, &phs);
    }

    return status;

    AROS_LIBFUNC_EXIT
} /* SetOwnerRelative */
