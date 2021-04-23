/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.

    Desc: Locks a file or directory.
*/

#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(BPTR, Lock,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name,       D1),
        AROS_LHA(LONG,         accessMode, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 14, Dos)

/*  FUNCTION
        Gets a lock on a file or directory. There may be more than one
        shared lock on a file but only one if it is an exclusive one.
        Locked files or directories may not be deleted.

    INPUTS
        name       - NUL terminated name of the file or directory.
        accessMode - One of SHARED_LOCK
                            EXCLUSIVE_LOCK

    RESULT
        Handle to the file or directory or 0 if the object couldn't be locked.
        IoErr() gives additional information in that case.

    NOTES
        The lock structure returned by this function is different
        from that of AmigaOS (in fact it is identical to a filehandle).
        Do not try to read any internal fields.

*****************************************************************************/

{
    AROS_LIBFUNC_INIT

    return LockRelative(NULL, name, accessMode);

    AROS_LIBFUNC_EXIT
} /* Lock */
