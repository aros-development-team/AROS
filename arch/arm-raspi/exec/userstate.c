/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: UserState() - Return to normal mode after changing things.
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH1(void, UserState,

/*  SYNOPSIS */
	AROS_LHA(APTR, superSP, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 26, Exec)

/*  FUNCTION
	Return to user mode after a call to SuperState().

    INPUTS
	superSP    -   The return value from SuperState()

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

    asm volatile (
        "       mov     r0, sp          \n"
        "       mov     sp, %[superSP]  \n"
        "       cps     %[mode_user]    \n"
        "       mov     sp, r0          \n"
        : : [superSP] "X" (superSP), [mode_user] "I" (CPUMODE_USER));

    AROS_LIBFUNC_EXIT
} /* UserState() */
