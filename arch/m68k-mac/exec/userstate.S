/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: userstate.S 13176 2002-02-09 06:21:26Z bergers $
*/

/*****************************************************************************
 
    NAME
 
 	AROS_LH1(ULONG, UserState,
 
    SYNOPSIS
        AROS_LHA(APTR, sysStack, D0)

    LOCATION
 	struct ExecBase *, SysBase, 26, Exec)
 
    FUNCTION
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

	#include "../include/aros/machine.i"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(UserState,Exec)

AROS_SLIB_ENTRY(UserState,Exec):
#ifndef DoRegisterCalls
	move.l	0x4.w,%a6      // Get SysBase
	move.l  4(%ssp),%d0    // get sysStack
#endif
	/*
	 * This is how the supervisor stack looks like (later usp)
	 *  0(%ssp)  (LONG) - return PC
	 *  4(%ssp)  (LONG) - sysStack - only there if not using register passing
	 */
	movem.l	%a1-%a2,-4(%ssp)

	move.l	%ssp,%a2         // remember for later

	move.l	%d0,%ssp         // load ssp with old ssp

	lea.l	return_part,%a1
	move.l	%a1,2(%ssp)      // store the return PC on ssp exception
	                         // frame, keep SR untouched
	move.l	%a2,%usp

	movem.l	-4(%a2),%a1-%a2  // restor a1/a2

	rte  // -> should now call return_part below

return_part:
	rts  // -> should lead me back to where I left off in the
             //    program flow
