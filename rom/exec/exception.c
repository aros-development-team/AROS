/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Exception - Perform a task exception.
    Lang: english
*/
#include <exec/execbase.h>
#include <aros/asmcall.h>

/*****i*************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH0(void, Exception,

/*  LOCATION */
	struct ExecBase *, SysBase, 11, Exec)

/*  FUNCTION
	Exception handler. This function is called by the dispatcher if
	a task exception has occured. It is called in the Disable()'d
	state so that all signals are still unchanged.

	TF_EXCEPT is still set and must be reset by task route.

	The exception code is called with the following parameters:

	    A1  -   Task->tc_ExceptData
	    D0  -   Mask of Flags which caused task exception.
	    A6  -   SysBase

    INPUTS

    RESULT

    NOTES
	This function is private. Do not call it from any program.

    EXAMPLE

    BUGS

    SEE ALSO
	Dispatch()

    INTERNALS
	Unlike in AmigaOS task function is called in user mode.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task *task = FindTask (NULL);
    BYTE nestCnt;
    ULONG flags;

    task->tc_Flags &= ~TF_EXCEPT;

    nestCnt = SysBase->IDNestCnt;
    SysBase->IDNestCnt = 0;

    while ((flags = (task->tc_SigExcept & task->tc_SigRecvd)))
    {
	task->tc_SigExcept ^= flags;
	task->tc_SigRecvd  ^= flags;

	Enable();

	/* Call the Exception */
	if(task->tc_ExceptCode)
	{
	    task->tc_SigExcept |= AROS_UFC3(ULONG, task->tc_ExceptCode,
		AROS_UFCA(APTR, task->tc_ExceptData, A1),
		AROS_UFCA(ULONG, flags, D0),
		AROS_UFCA(struct ExecBase *, SysBase, A6));
	}

	Disable();
    }

    SysBase->IDNestCnt = nestCnt;

    AROS_LIBFUNC_EXIT
} /* Exception */
