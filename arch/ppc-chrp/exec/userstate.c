/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id: userstate.c 28969 2008-07-03 18:50:17Z schulz $

    Desc: UserState() - Return to normal mode after changing things.
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <proto/exec.h>
#include <asm/mpc5200b.h>

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

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    wrmsr(rdmsr() | (MSR_PR));

    sysStack = 0;   /* Get rid of the compiler warning */

    AROS_LIBFUNC_EXIT
} /* UserState() */
