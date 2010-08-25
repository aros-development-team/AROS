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

	TF_EXCEPT is still set and must be reset by this route.

	The exception code is called with the following parameters:

	    A1  -   Task->tc_ExceptData
	    D0  -   Mask of Flags which caused this exception.
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
	Unlike in AmigaOS this function is called in user mode.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task * this = FindTask (NULL);
    BYTE nestCnt;
    ULONG flags;

    this->tc_Flags &= ~TF_EXCEPT;

    nestCnt = SysBase->IDNestCnt;
    SysBase->IDNestCnt = 0;

    while ((flags = (this->tc_SigExcept & this->tc_SigRecvd)))
    {
	flags ^= this->tc_SigExcept;
	flags ^= this->tc_SigRecvd;

	Enable ();

	/* Call the Exception with the new AROS ASMCALL macros */
	if( this->tc_ExceptCode != NULL)
	{
	    this->tc_SigExcept = AROS_UFC3(ULONG, this->tc_ExceptCode,
		AROS_UFCA(APTR, this->tc_ExceptData, A1),
		AROS_UFCA(ULONG, flags, D0),
		AROS_UFCA(struct ExecBase *, SysBase, A6));
	}
	else
	{
	    /*
		We don't have an exception handler, we shouldn't have
		any exceptions. Clear them.
	    */

	    this->tc_SigExcept = 0;
	}
	Disable ();
    }

    SysBase->IDNestCnt = nestCnt;

    AROS_LIBFUNC_EXIT
} /* Exception */
