/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1996/10/24 15:50:57  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:56:08  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:19  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/execbase.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	AROS_LH2(ULONG, SetExcept,

/*  SYNOPSIS */
	AROS_LHA(ULONG, newSignals, D0),
	AROS_LHA(ULONG, signalSet,  D1),

/*  LOCATION */
	struct ExecBase *, SysBase, 52, Exec)

/*  FUNCTION
	Change the mask of signals causing a task exception.

    INPUTS
	newSignals - Set of signals causing the exception.
	signalSet  - Mask of affected signals.

    RESULT
	Old mask of signals causing a task exception.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AllocSignal(), FreeSignal(), Wait(), SetSignal(), Signal()

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task *me;
    ULONG old;

    /* Get pointer to current task */
    me=SysBase->ThisTask;

    /* Protect mask of sent signals and task lists */
    Disable();

    /* Get returncode */
    old=me->tc_SigExcept;

    /* Change exception mask */
    me->tc_SigExcept=(old&~signalSet)|(newSignals&signalSet);

    /* Does this change include an exception? */
    if(me->tc_SigExcept&me->tc_SigRecvd)
    {
	/* Yes. Set the exception flag. */
	me->tc_Flags|=TF_EXCEPT;

	/* Are taskswitches allowed? (Don't count own Disable() here) */
	if(SysBase->TDNestCnt>=0||SysBase->IDNestCnt>0)
	    /* No. Store it for later. */
	    SysBase->AttnResched|=0x80;
	else
	    /* Switches are allowed. Force a rescedule. */
	    Switch();
    }
    Enable();

    return old;
    AROS_LIBFUNC_EXIT
}


