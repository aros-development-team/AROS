/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/08/13 13:56:02  digulla
    Replaced __AROS_LA by __AROS_LHA
    Replaced some __AROS_LH*I by __AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:12  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH1(void, FreeSignal,

/*  SYNOPSIS */
	__AROS_LHA(LONG, signalNum, D0),

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

    HISTORY

******************************************************************************/
{
    __AROS_FUNC_INIT

    if(signalNum!=-1)
    {
	/* Nobody guarantees that the compiler will make it atomic. */
	Forbid();

	/* Clear the bit */
	SysBase->ThisTask->tc_SigAlloc&=~(1<<signalNum);
	Permit();
    }
    __AROS_FUNC_EXIT
}

