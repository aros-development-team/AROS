/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/debug.h>

#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH0(STRPTR, GetArgStr,

/*  SYNOPSIS */

/*  LOCATION */
        struct DosLibrary *, DOSBase, 89, Dos)

/*  FUNCTION
        Returns a pointer to the argument string passed to the current
        process at startup.

    INPUTS

    RESULT
        Pointer to argument string.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        SetArgStr(), RunCommand()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Get pointer to process structure */
    struct Process *me=(struct Process *)FindTask(NULL);

    ASSERT_VALID_PROCESS(me);

    /* Nothing spectacular */
    return me->pr_Arguments;
    AROS_LIBFUNC_EXIT
} /* GetArgStr */
