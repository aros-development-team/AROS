/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: UserState() - Return to normal mode after changing things.
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH1(void, UserState,

/*  SYNOPSIS */
	AROS_LHA(APTR, sysStack, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 26, Exec)

/*  FUNCTION
	Return to user mode after a call to SuperState().

    INPUTS
	sysStack    -   The return value from SuperState()

    RESULT
	The system will be back to normal.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	SuperState(), Supervisor()

    INTERNALS
	Undo SuperState()

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* By default there is nothing to do. */

	__asm__ __volatile
		(
			"movl %%esp,0x0c(%%eax)\n\t"	/* put USP onto exception stack frame */
			"movl %%eax,%%esp\n\t"			/* SSP = SP */
			"leal	1f,%%eax\n\t"
			"movl	%%eax,(%%esp)\n\t"		/* return at this address */
			"iret\n"
			"1:"
			:
			:"a"(sysStack)
		);

    AROS_LIBFUNC_EXIT
} /* UserState() */
