/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
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

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* Get pointer to process structure */
    struct Process *me=(struct Process *)FindTask(NULL);

    /* Nothing spectacular */
    return me->pr_Arguments;
    AROS_LIBFUNC_EXIT
} /* GetArgStr */
