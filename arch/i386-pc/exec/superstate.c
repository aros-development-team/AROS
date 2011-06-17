/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: SuperState() - Switch the processor into a higher plane.
    Lang: english
*/

void exec_SuperState(); /* defined in corelow.S */

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

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    /*  Again see a real implementation for more information.

	You will have to change into supervisor mode, and then change the
	stack to the original user stack before returning.
    */

    /* We have to return something. NULL is an invalid address for a
       stack, so it could be used to say that this function does
       nothing.
    */    
    return (APTR)Supervisor((ULONG_FUNC)exec_SuperState);

    AROS_LIBFUNC_EXIT
} /* SuperState() */
