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

	AROS_LH1(BPTR, SelectError,

/*  SYNOPSIS */
	AROS_LHA(BPTR, fh, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 144, Dos)

/*  FUNCTION
	Sets the current error stream returned by Error() to a new
	value. Returns the old error stream.

    INPUTS
	fh - New error stream.

    RESULT
	Old error stream handle.

    NOTES
	This function is AROS specific
	
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

    /* Nothing spectacular */
    old=me->pr_CES;
    me->pr_CES=fh;
    return old;
    AROS_LIBFUNC_EXIT
} /* SelectInput */
