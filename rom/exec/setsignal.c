/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/08/13 13:56:08  digulla
    Replaced __AROS_LA by __AROS_LHA
    Replaced some __AROS_LH*I by __AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:19  digulla
    Added standard header for all files

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
	__AROS_LHA(ULONG, newSignals, D0),
	__AROS_LHA(ULONG, signalSet,  D1),

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


