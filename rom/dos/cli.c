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

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Get pointer to process structure */
    struct Process *me = (struct Process *) FindTask(NULL);
    
    ASSERT_VALID_PROCESS(me);

    /* Make sure this is a process */
    if (me->pr_Task.tc_Node.ln_Type == NT_PROCESS)
    {
        /* Nothing spectacular */
        return (struct CommandLineInterface *) BADDR(me->pr_CLI);
    }
    else
    {
        return NULL;
    }
    
    AROS_LIBFUNC_EXIT
} /* Cli */
