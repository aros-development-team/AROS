/*
    Copyright (C) 1995-2008, The AROS Development Team. All rights reserved.

    Desc: Change the date of a file.
*/

#include <string.h>
#include <aros/debug.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>
#include <dos/dos.h>

        AROS_LH3(BOOL, SetFileDateRelative,

/*  SYNOPSIS */
        AROS_LHA(BPTR,                     lock, D1),
        AROS_LHA(CONST_STRPTR,             name, D2),
        AROS_LHA(const struct DateStamp *, date, D3),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 234, Dos)

/*  FUNCTION
        Change the modification time of a file or directory.

    INPUTS
        name - name of the file
        date - new file time

    RESULT
        Boolean success indicator. IoErr() gives additional information upon
        failure.

    NOTES
        This call is AROS-specific.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PacketHelperStruct phs;
    LONG status = DOSFALSE;

    D(bug("[SetFileDateRelative] lock=0x%p '%s' %x\n", lock, name, date));

    if (getpacketinfo(DOSBase, lock, name, &phs)) {
        status = dopacket4(DOSBase, NULL, phs.port, ACTION_SET_DATE, (IPTR)NULL, phs.lock, phs.name, (IPTR)date);
        freepacketinfo(DOSBase, &phs);
    }

    return status;

    AROS_LIBFUNC_EXIT
} /* SetFileDateRelative */
