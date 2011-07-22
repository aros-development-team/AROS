/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: SuperState() - Switch the processor into a higher plane.
    Lang: english
*/

#include <aros/debug.h>
#include <proto/kernel.h>

#include "exec_intern.h"

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

#if (AROS_FLAVOUR & AROS_FLAVOUR_STANDALONE)
    /*
     * This part works only on native AROS.
     * Hosted ports are running in a virtual machine with only single privilege
     * level available, so this function will simply return NULL.
     * cpu_SuperState() is an architecture-specific helper code written in asm.
     */

    int super = KrnIsSuper();
    
    D(bug("[SuperState] Current supervisor mode: %d\n", super));

    if (!super)
    {
        APTR ssp = Supervisor(cpu_SuperState);
        
        D(bug("[SuperState] Saved SP 0x%p\n", ssp));
        return ssp;
    }
#endif

    return NULL;

    AROS_LIBFUNC_EXIT
} /* SuperState() */
