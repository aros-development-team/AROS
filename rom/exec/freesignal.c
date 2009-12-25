/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Free a signal.
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(void, FreeSignal,

/*  SYNOPSIS */
	AROS_LHA(LONG, signalNum, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 56, Exec)

/*  FUNCTION
	Free a signal allocated with AllocSignal().

    INPUTS
	signalNum - Number of the signal to free or -1 to do nothing.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AllocSignal(), Signal(), Wait()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if(signalNum!=-1)
    {
        /* No more atomic problem - i beleive THIS is atomic. - sonic */
        struct Task *me = SysBase->ThisTask;

	/* Clear the bit */
	me->tc_SigAlloc &= ~(1<<signalNum);
    }
    AROS_LIBFUNC_EXIT
} /* FreeSignal() */

