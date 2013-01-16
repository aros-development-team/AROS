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

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    asm("cps #0x1f\n");	/* switch to system mode */

    sysStack = 0;   /* Get rid of the compiler warning */

    AROS_LIBFUNC_EXIT
} /* UserState() */
