/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: dos.library function DupLock()
    Lang: english
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH1(BPTR, DupLock,

/*  SYNOPSIS */
        AROS_LHA(BPTR, lock, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 16, Dos)

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

    struct FileLock *fl = (struct FileLock *)BADDR(lock);
    BPTR ret;

    if (lock == BNULL)
        return BNULL;
    ret = (BPTR)dopacket1(DOSBase, NULL, fl->fl_Task, ACTION_COPY_DIR, lock);
    D(bug("[DupLock] %x -> %x\n", fl, BADDR(ret)));
    return ret;

    AROS_LIBFUNC_EXIT
} /* DupLock */
