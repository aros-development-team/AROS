/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:27:17  digulla
    Added copyright notics and made headers conform

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH2(ULONG, SetSignal,

/*  SYNOPSIS */
	__AROS_LA(ULONG, newSignals, D0),
	__AROS_LA(ULONG, signalSet,  D1),

/*  LOCATION */
	struct ExecBase *, SysBase, 51, Exec)

/*  FUNCTION
	Change or read the set of signals sent to the current task.

    INPUTS
	newSignals - New values for the signals.
	signalSet  - Mask of signals affected by 'newSignals'.

    RESULT
	Old signal set.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AllocSignal(), FreeSignal(), Wait(), Signal(), SetExcept()

    INTERNALS

    HISTORY

******************************************************************************/
{
    __AROS_FUNC_INIT

    ULONG *sig;
    ULONG old;

    /* Protect the signal mask against access by other tasks. */
    Disable();

    /* Get address */
    sig=&SysBase->ThisTask->tc_SigRecvd;

    /* Change only the bits in 'mask' */
    old=*sig;
    *sig=(old&~signalSet)|(newSignals&signalSet);

    Enable();

    return old;
    __AROS_FUNC_EXIT
}


