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

	AROS_LH0(struct CommandLineInterface *, Cli,

/*  SYNOPSIS */

/*  LOCATION */
	struct DosLibrary *, DOSBase, 82, Dos)

/*  FUNCTION
	Returns a pointer to the CLI structure of the current process.

    INPUTS

    RESULT
	Pointer to CLI structure.

    NOTES
	Do not use this function to test if the process was started from
	the shell. Check pr_CLI instead.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* Get pointer to process structure */
    struct Process *me = (struct Process *)FindTask(NULL);

    /* Nothing spectacular */
    return (struct CommandLineInterface *)BADDR(me->pr_CLI);

    AROS_LIBFUNC_EXIT
} /* Cli */
