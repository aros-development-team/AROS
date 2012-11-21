/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Set the owner of a file.
    Lang: English
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
        name        --  name of the file
        owner_info  --  (UID << 16) + GID

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

    struct PacketHelperStruct phs;
    LONG status = DOSFALSE;

    D(bug("[SetOwner] '%s' %x\n", name, owner_info));

    if (getpacketinfo(DOSBase, name, &phs)) {
        status = dopacket4(DOSBase, NULL, phs.port, ACTION_SET_OWNER, (IPTR)NULL, phs.lock, phs.name, (IPTR)owner_info);
        freepacketinfo(DOSBase, &phs);
    }

    return status;

    AROS_LIBFUNC_EXIT
} /* SetOwner */
