/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************
 
    NAME
 
 	AROS_LH0(ULONG, SuperState,
 
    LOCATION
 	struct ExecBase *, SysBase, 25, Exec)
 
    FUNCTION
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

	.align 4
	.globl Exec_SuperState
Exec_SuperState:
	mov	ip, sp
	stmfd	sp!, {r4, fp, ip, lr, pc}
	sub	fp, ip, #4

	mov	r4, r0

	mov	r0, #0          // 1st param
	mov	r1, r0          // 2nd param
	move	r2, r4          // SysBase
	mov	lr, pc
	ldr	pc, [r4, #-96]	// call SetSR

	// check whether I am already in superstate?

	// I am already in superstate!
	ldmea	fp, {r4, fp, sp, pc}

goon:	

	.align 4
	.globl _sys_SuperState
_sys_SuperState:
