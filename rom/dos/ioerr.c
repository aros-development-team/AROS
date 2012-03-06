/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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

	AROS_LH0(SIPTR, IoErr,

/*  SYNOPSIS */

/*  LOCATION */
	struct DosLibrary *, DOSBase, 22, Dos)

/*  FUNCTION
	Get the dos error code for the current process.

    INPUTS

    RESULT
	Error code.

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
    return me->pr_Result2;
    AROS_LIBFUNC_EXIT
} /* IoErr */
