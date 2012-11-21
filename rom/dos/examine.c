/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: dos.library function Examine().
    Lang: English
*/

#include <aros/debug.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(LONG, Examine,

/*  SYNOPSIS */
        AROS_LHA(BPTR,                   lock, D1),
        AROS_LHA(struct FileInfoBlock *, fib,  D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 17, Dos)

/*  FUNCTION

    Fill in a FileInfoBlock structure concerning a file or directory 
    associated with a particular lock.

    INPUTS

    lock  --  lock to examine
    fib   --  FileInfoBlock where the result of the examination is stored

    RESULT

    A boolean telling whether the operation was successful or not.

    NOTES

    FileInfoBlocks should be allocated with AllocDosObject(). You may make
    a copy of the FileInfoBlock but, however, this copy may NOT be passed
    to ExNext()!

    EXAMPLE

    BUGS

    SEE ALSO

    Lock(), UnLock(), ExNext(), AllocDosObject(), ExAll(), <dos/dos.h>

    INTERNALS

*****************************************************************************/

{
    AROS_LIBFUNC_INIT

    struct FileLock *fl = BADDR(lock);
    LONG ret;

    ASSERT_VALID_PTR_OR_NULL(BADDR(lock));
    ASSERT_VALID_FILELOCK(lock);

    D(bug("[Examine] lock=%x fib=%x\n", fl, fib));
    ret = dopacket2(DOSBase, NULL, fl->fl_Task, ACTION_EXAMINE_OBJECT, lock, MKBADDR(fib));
    if (ret) {
        fixfib(fib);
        D(bug("[Examine] '%s'\n", fib->fib_FileName));
    }
    return ret;

    AROS_LIBFUNC_EXIT
} /* Examine */
