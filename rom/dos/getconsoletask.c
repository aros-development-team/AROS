/*
    Copyright (C) 1995-1998 AROS - The Amiga Research OS
    $Id$

    Desc: Get the "task" belonging to the process's console.
    Lang: english
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <dos/dosextens.h>
#include <proto/dos.h>

	AROS_LH0(struct MsgPort *, GetConsoleTask,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct DosLibrary *, DOSBase, 85, Dos)

/*  FUNCTION
	Return the console handler for the current Process. The return
	type depends upon whether AROS is running binary compatible.

    INPUTS
	None.

    RESULT
	The address of the console handler, or NULL if none is set.

    NOTES
	You will only get NULL from this call if you call it on a Task,
	or when the Process is not attached to a console.

    EXAMPLE

    BUGS

    SEE ALSO
	SetConsoleTask()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    struct Process *pr = (struct Process *)FindTask(NULL);
    return BADDR(pr->pr_ConsoleTask);

    AROS_LIBFUNC_EXIT
} /* GetConsoleTask */
