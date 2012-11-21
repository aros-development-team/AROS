/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include <aros/debug.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH1(BPTR, CurrentDir,

/*  SYNOPSIS */
        AROS_LHA(BPTR, lock, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 21, Dos)

/*  FUNCTION
        Sets a new directory as the current directory. Returns the old one.
        0 is valid in both cases and represents the boot filesystem.

    INPUTS
        lock - Lock for the new current directory.

    RESULT
        Old current directory.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Get pointer to process structure */
    struct Process *me=(struct Process *)FindTask(NULL);
    BPTR old;

    ASSERT_VALID_PROCESS(me);

    ASSERT_VALID_PTR_OR_NULL(BADDR(lock));

    /* Nothing spectacular */
    old=me->pr_CurrentDir;
    me->pr_CurrentDir=lock;
    ASSERT_VALID_PTR_OR_NULL(BADDR(old));
    return old;
    AROS_LIBFUNC_EXIT
} /* CurrentDir */
