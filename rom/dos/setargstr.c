/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"
#include <dos/dosextens.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(STRPTR, SetArgStr,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, string, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 90, Dos)

/*  FUNCTION
	Sets the arguments to the current process. The arguments must be
	reset to the original value before process exit.

    INPUTS
	string	-   The new argument string. (A C string).

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
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    STRPTR oldStr;
    struct Process *pr = (struct Process *)FindTask(NULL);
    oldStr = pr->pr_Arguments;
    pr->pr_Arguments = string;

    return oldStr;

    AROS_LIBFUNC_EXIT
} /* SetArgStr */
