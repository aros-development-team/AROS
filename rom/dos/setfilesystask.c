/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Set the current filesystem handler for a process.
    Lang: english
*/
#include <aros/debug.h>

#include "dos_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <dos/dosextens.h>
#include <proto/dos.h>

        AROS_LH1(struct MsgPort *, SetFileSysTask,

/*  SYNOPSIS */
        AROS_LHA(struct MsgPort *, task, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 88, Dos)

/*  FUNCTION
        Set the default filesystem handler for the current process,
        the old filesystem handler will be returned.

    INPUTS
        task - The new filesystem handler.

    RESULT
        The old filesystem handler.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        GetFileSysTask()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Process *pr = (struct Process *)FindTask(NULL);
    APTR old;

    ASSERT_VALID_PROCESS(pr);

    old = pr->pr_FileSystemTask;
    pr->pr_FileSystemTask = task;
    return old;

    AROS_LIBFUNC_EXIT
} /* SetFileSysTask */
