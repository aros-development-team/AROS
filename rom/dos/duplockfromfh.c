/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: dos.library function DupLockFromFH()
    Lang: english
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH1(BPTR, DupLockFromFH,

/*  SYNOPSIS */
        AROS_LHA(BPTR, handle, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 62, Dos)

/*  FUNCTION
        Clone a lock on a file or directory. This will only work on shared
        locks.

    INPUTS
        lock - Old lock.

    RESULT
        The new lock or NULL in case of an error. IoErr() will give additional
        information in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct FileHandle *fh = BADDR(handle);
    BPTR ret;

    if (handle == BNULL)
        return BNULL;

    /* Special case for NIL: */
    if (fh->fh_Type == BNULL) {
        struct FileLock *fl = AllocMem(sizeof(struct FileLock), MEMF_PUBLIC | MEMF_CLEAR);
        return MKBADDR(fl);
    }

    ret = (BPTR)dopacket1(DOSBase, NULL, fh->fh_Type, ACTION_COPY_DIR_FH, fh->fh_Arg1);
    D(bug("[DupLockFromFH] %x -> %x\n", fh, BADDR(ret)));
    return ret;

    AROS_LIBFUNC_EXIT
} /* DupLockFromFH */
