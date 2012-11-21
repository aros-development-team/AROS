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

        AROS_LH0(BPTR, Input,

/*  SYNOPSIS */

/*  LOCATION */
        struct DosLibrary *, DOSBase, 9, Dos)

/*  FUNCTION
        Returns the current input stream or 0 if there is no current
        input stream.

    INPUTS

    RESULT
        Input stream handle.

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

    ASSERT_VALID_PROCESS(me);

    /* Nothing spectacular */
    return me->pr_CIS;
    AROS_LIBFUNC_EXIT
} /* Input */
