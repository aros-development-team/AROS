/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1997/01/01 03:46:16  ldp
    Committed Amiga native (support) code

    Changed clib to proto

    Revision 1.6  1996/12/10 13:51:54  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.5  1996/10/24 15:50:57  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:56:08  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:19  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH2(ULONG, SetSignal,

/*  SYNOPSIS */
	AROS_LHA(ULONG, newSignals, D0),
	AROS_LHA(ULONG, signalSet,  D1),

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
    AROS_LIBFUNC_INIT

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
    AROS_LIBFUNC_EXIT
}


