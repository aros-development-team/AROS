/*
    $Id$
    $Log$
    Revision 1.1  1996/07/28 16:37:24  digulla
    Initial revision

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


