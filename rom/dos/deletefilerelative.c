/*
    Copyright (C) 1995-2013, The AROS Development Team. All rights reserved.

    Desc: Delete a file or directory.
*/
#include <aros/debug.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(BOOL, DeleteFileRelative,

/*  SYNOPSIS */
        AROS_LHA(BPTR,         lock, D1),
        AROS_LHA(CONST_STRPTR, name, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 232, Dos)

/*  FUNCTION
        Tries to delete a file or directory by a given name.
        May fail if the file is in use or protected from deletion.

    INPUTS
        name - NUL terminated name.

    RESULT
        != 0 if the file is gone, 0 if is still there.
        IoErr() gives additional information in that case.

    NOTES
        This call is AROS-specific.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG status = 0;
    struct PacketHelperStruct phs;

    D(bug("[DeleteFileRelative] lock=0x%p '%s'\n", lock, name));

    if (getpacketinfo(DOSBase, lock, name, &phs)) {
        status = dopacket2(DOSBase, NULL, phs.port, ACTION_DELETE_OBJECT, phs.lock, phs.name);
        freepacketinfo(DOSBase, &phs);
    }

    return status;

    AROS_LIBFUNC_EXIT
} /* DeleteFileRelative */
