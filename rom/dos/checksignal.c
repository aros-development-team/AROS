/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Checks for signals in a mask.
    Lang: english
*/
#include <exec/tasks.h>
#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH1(LONG, CheckSignal,

/*  SYNOPSIS */
        AROS_LHA(LONG, mask, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 132, Dos)

/*  FUNCTION
        Checks the current task to see if any of the signals specified in
        the mask have been set. The mask of all signals which were set is
        returned. The signals specified in the mask will be cleared.

    INPUTS
        mask - The signal mask to check.

    RESULT
        The mask of all signals which were set.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG rcvd;

    /* Get pointer to current task structure */
    struct Task *me = FindTask(NULL);

    /* Protect the signal mask against access by other tasks. */
    Disable();

    /* Get active signals specified in mask */
    rcvd = me->tc_SigRecvd & mask;

    /* And clear them. */
    me->tc_SigRecvd &= ~mask;

    /* All done. */
    Enable();

    return rcvd;

    AROS_LIBFUNC_EXIT
} /* CheckSignal */
