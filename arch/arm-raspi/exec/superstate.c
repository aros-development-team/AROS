/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: SuperState() - Switch the processor into a higher plane.
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH0(APTR, SuperState,

/*  LOCATION */
	struct ExecBase *, SysBase, 25, Exec)

/*  FUNCTION
	Enter supervisor mode (like Supervisor()), but return on the user
	stack. This will mean that the user stack variables are still there.
	A call to UserState() will end this mode.

    INPUTS
	None.

    RESULT
	The old supervisor stack. This must be passed to UserState(). If the
	processor was already in supervisor mode, then this function will
	return NULL. In that case do NOT call UserState().

    NOTES
	This is not a good function to use, it has limited scope, and will
	probably be even less useful in the future.

    EXAMPLE

    BUGS
	You can easily cause your system to cease operating normally.

    SEE ALSO
	Supervisor(), UserState()

    INTERNALS
	For extra details see Supervisor().

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    register unsigned int superSP;

    asm volatile (
        "       stmfd   sp!, {lr}               \n"
        "       mov     r1, sp                  \n"
        "       swi     %[swi_no]               \n"
        "       mov     %[superSP], sp          \n"
        "       mov     sp, r1                  \n"
        "       ldmfd   sp!, {lr}               \n"
        : [superSP] "=r" (superSP)
        : [swi_no] "I" (6 /*SC_SUPERSTATE*/) : "r1"
    );

    return superSP;

    AROS_LIBFUNC_EXIT
} /* SuperState() */
