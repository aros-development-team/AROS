/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH0(BPTR, Error,

/*  SYNOPSIS */

/*  LOCATION */
	struct DosLibrary *, DOSBase, 142, Dos)

/*  FUNCTION
	Returns the current error stream or 0 if there is no current
	input stream.

    INPUTS

    RESULT
	Error stream handle.

    NOTES
	This function is AROS specific

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Get pointer to process structure */
    struct Process *me=(struct Process *)FindTask(NULL);

    /* Nothing spectacular */
    return me->pr_CES;
    AROS_LIBFUNC_EXIT
} /* Input */
