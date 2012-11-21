/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/debug.h>
#include "dos_intern.h"
#include <dos/dos.h>

/*****************************************************************************

    NAME */
#include <exec/types.h>
#include <proto/dos.h>

        AROS_LH1(BPTR, ParentDir,

/*  SYNOPSIS */
        AROS_LHA(BPTR, lock, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 35, Dos)

/*  FUNCTION
        Returns a lock to the parent directory of the supplied lock.

    INPUTS
        lock - Lock to get parent directory of.

    RESULT
        Returns a lock to the parent directory or NULL, in which case the 
        supplied lock has no parent directory (because it is the root 
        directory) or an error occured. IoErr() returns 0 in the former case 
        and a different value on error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct FileLock *fl = BADDR(lock);
    D(bug("[ParentDir] lock=%x\n", lock));
    return (BPTR)dopacket1(DOSBase, NULL, fl->fl_Task, ACTION_PARENT, lock);
 
    AROS_LIBFUNC_EXIT
} /* ParentDir */
