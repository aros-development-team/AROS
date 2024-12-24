/*
    Copyright (C) 1995-2013, The AROS Development Team. All rights reserved.

    Desc: Create a new directory.
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

        AROS_LH2(BPTR, CreateDirRelative,

/*  SYNOPSIS */
        AROS_LHA(BPTR,         lock, D1),
        AROS_LHA(CONST_STRPTR, name, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 231, Dos)

/*  FUNCTION
        Creates a new directory under the given name. If all went well, an
        exclusive lock on the new diretory is returned.

    INPUTS
        name - NUL terminated name.

    RESULT
        Exclusive lock to the new directory or 0 if it couldn't be created.
        IoErr() gives additional information in that case.

    NOTES
        This call is AROS-specific.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BPTR lock = BNULL;
    struct PacketHelperStruct phs;

    D(bug("[CreateDirRelative] lock=0x%p '%s'\n", lock, name));

    if (getpacketinfo(DOSBase, lock, name, &phs)) {
        lock = (BPTR)dopacket2(DOSBase, NULL, phs.port, ACTION_CREATE_DIR, phs.lock, phs.name);
        freepacketinfo(DOSBase, &phs);
    }

    return lock;

    AROS_LIBFUNC_EXIT
} /* CreateDirRelative */

