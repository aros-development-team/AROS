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

        AROS_LH1(BPTR, SelectOutput,

/*  SYNOPSIS */
        AROS_LHA(BPTR, fh, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 50, Dos)

/*  FUNCTION
        Sets the current output stream returned by Output() to a new
        value. Returns the old output stream.

    INPUTS
        fh - New output stream.

    RESULT
        Old output stream handle.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BPTR old;

    /* Get pointer to process structure */
    struct Process *me=(struct Process *)FindTask(NULL);

    ASSERT_VALID_PROCESS(me);

    /* Nothing spectacular */
    old=me->pr_COS;
    me->pr_COS=fh;
    return old;
    AROS_LIBFUNC_EXIT
} /* SelectOutput */
