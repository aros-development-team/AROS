/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Set the console handler for the current process.
    Lang: english
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <dos/dosextens.h>
#include <proto/dos.h>

	AROS_LH1(struct MsgPort *, SetConsoleTask,

/*  SYNOPSIS */
	AROS_LHA(struct MsgPort *, handler, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 86, Dos)

/*  FUNCTION
	Set the console handler for the current process, and return the
	old handler.

    INPUTS
	handler		- The new console handler for the process.

    RESULT
	The address of the old handler.

    NOTES
	The use of Task in the name is because historically filesystem
	handlers were tasks (instead of Devices).

    EXAMPLE

    BUGS

    SEE ALSO
	GetConsoleTask()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    APTR old;
    struct Process *pr;
    
    pr = (struct Process *)FindTask(NULL);
    old = pr->pr_ConsoleTask;
    pr->pr_ConsoleTask = MKBADDR(handler);

    return BADDR(old);

    AROS_LIBFUNC_EXIT
} /* SetConsoleTask */
