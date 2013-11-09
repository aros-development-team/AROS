/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/debug.h>

#include "dos_intern.h"
#include <dos/dosextens.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH1(STRPTR, SetArgStr,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, string, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 90, Dos)

/*  FUNCTION
        Sets the arguments to the current process. The arguments must be
        reset to the original value before process exit.

    INPUTS
        string - The new argument string (a C string).

    RESULT
        The address of the previous argument string. May be NULL.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        GetArgStr(), RunCommand()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    STRPTR oldStr;
    struct Process *pr = (struct Process *)FindTask(NULL);
    ASSERT_VALID_PROCESS(pr);
    oldStr = pr->pr_Arguments;
    pr->pr_Arguments = (STRPTR)string;

    return oldStr;

    AROS_LIBFUNC_EXIT
} /* SetArgStr */
