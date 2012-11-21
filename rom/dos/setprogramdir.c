/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Set the home directory for a program (PROGDIR:)
    Lang: english
*/
#include <aros/debug.h>

#include "dos_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH1(BPTR, SetProgramDir,

/*  SYNOPSIS */
        AROS_LHA(BPTR, lock, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 99, Dos)

/*  FUNCTION
        This function will set a shared lock on the directory that the
        current program was loaded from. This can be accessed through
        the path PROGDIR:. The use of this path is to allow the program
        to easily access files which are supplied with the program.

    INPUTS
        lock    -   The lock to set as the new program directory. NULL
                    is a valid value.

    RESULT
        This function will return the old program directory lock.

    NOTES
        This function will not duplicate the lock, so you should not
        free the lock.

    EXAMPLE

    BUGS

    SEE ALSO
        GetProgramDir()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Process *pr;
    BPTR oldLock;

    pr = (struct Process *)FindTask(NULL);

    ASSERT_VALID_PROCESS(pr);

    oldLock = pr->pr_HomeDir;
    pr->pr_HomeDir = lock;

    return oldLock;

    AROS_LIBFUNC_EXIT
} /* SetProgramDir */
