/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Get the "task" belonging to the process's console.
    Lang: English
*/

#include "dos_intern.h"
#include <proto/exec.h>

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

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Process *me = (struct Process *)FindTask(NULL);

    if (__is_task(me))
    {
        return NULL;
    }
    
    return (struct MsgPort *)me->pr_ConsoleTask;

    AROS_LIBFUNC_EXIT
} /* GetConsoleTask */
