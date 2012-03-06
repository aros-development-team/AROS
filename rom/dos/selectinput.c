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

	AROS_LH1(BPTR, SelectInput,

/*  SYNOPSIS */
	AROS_LHA(BPTR, fh, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 49, Dos)

/*  FUNCTION
	Sets the current input stream returned by Input() to a new
	value. Returns the old input stream.

    INPUTS
	fh - New input stream.

    RESULT
	Old input stream handle.

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
    old=me->pr_CIS;
    me->pr_CIS=fh;
    return old;
    AROS_LIBFUNC_EXIT
} /* SelectInput */
