/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Set the console handler for the current process.
    Lang: english
*/
#include <aros/debug.h>

#include "dos_intern.h"
#include <proto/exec.h>

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

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    APTR old;
    struct Process *pr;
    
    pr = (struct Process *)FindTask(NULL);
    ASSERT_VALID_PROCESS(pr);

    old = pr->pr_ConsoleTask;
    pr->pr_ConsoleTask = handler;

    return old;

    AROS_LIBFUNC_EXIT
} /* SetConsoleTask */
