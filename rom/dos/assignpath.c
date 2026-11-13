/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Create a non-binding (path) assign.
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(BOOL, AssignPath,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name, D1),
        AROS_LHA(CONST_STRPTR, path, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 104, Dos)

/*  FUNCTION
        Create an assign for the given name, which will be resolved upon
        each reference to it. There will be no permanent lock kept on the
        specified path. This way you can create assigns to unmounted volumes
        which will only be requested when accessed. Also, using AssignPath()
        to assign C: to df0:c would make references go to to df0:c even if
        you change the disk.

    INPUTS
        name  -- NULL terminated name of the assign.
        path  -- NULL terminated path to be resolved on each reference.

    RESULT
        != 0 in case of success, 0 on failure. IoErr() gives additional
        information in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        AssignAdd(), AssignLock(), AssignLate(), Open()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return assign_path(name, path, DLT_NONBINDING, DOSBase);

    AROS_LIBFUNC_EXIT
} /* AssignPath */
