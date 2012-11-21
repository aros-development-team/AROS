/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Change the mode of a filehandle or lock.
    Lang: English
*/
#include <aros/debug.h>
#include <proto/exec.h>
#include <dos/dosextens.h>

#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH3(BOOL, ChangeMode,

/*  SYNOPSIS */
        AROS_LHA(ULONG, type,    D1),
        AROS_LHA(BPTR,  object,  D2),
        AROS_LHA(ULONG, newmode, D3),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 75, Dos)

/*  FUNCTION
        Try to change the access mode of a lock or filehandle.

    INPUTS
        type    - CHANGE_FH or CHANGE_LOCK.
        object  - Filehandle or lock.
        newmode - New mode, either SHARED_LOCK or EXCLUSIVE_LOCK.

    RESULT
        != 0 if all went well, otherwise 0. IoErr() gives additional
        information in the latter case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        Since filehandles and locks are identical under AROS the type
        argument is ignored.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Get pointer to filehandle */
    struct FileHandle *fh = (struct FileHandle*)BADDR(object);
    struct FileLock *fl = (struct FileLock*)BADDR(object);
    LONG ret;
    
    if (type != CHANGE_LOCK && type != CHANGE_FH) {
        SetIoErr(ERROR_BAD_NUMBER);
        return FALSE;
    }
    D(bug("[ChangeMode] %d %x %d\n", type, fh, newmode));
    ret = dopacket3(DOSBase, NULL, type == CHANGE_LOCK ? fl->fl_Task : fh->fh_Type, ACTION_CHANGE_MODE, type, object, newmode);
    return ret;

    AROS_LIBFUNC_EXIT
} /* ChangeMode */
