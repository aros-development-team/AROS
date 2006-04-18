/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: superstate.S 13176 2002-02-09 06:21:26Z bergers $
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

	#include "../include/aros/machine.i"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(SuperState,Exec)

AROS_SLIB_ENTRY(SuperState,Exec):
#if DoRegisterCalls
	move.l	$4,%a6         // Get SysBase
#endif
	movem.l	%d0/%d1,-(%sp)
	clr.l	%d0
	clr.l	%d1
	jmp	SetSR(%a6)     // %d0 will contain current state
	                       // (supervisor/user)
	and.w	0x2000,%d0     // this should test the user or superstate
                               // bit
	beq	go_on
	movem.l	(%sp)+,%d0/%d1 // already in superstate
	clr.l	%d0
	rts

go_on:	movem.l (%sp)+,%d0/%d1 // switch to superstate

	move.l  %d7,-(%sp)
	move.l  #12,%d7     // #12 => call _sys_trap1_SuperState
	trap    #1
	move.l  (%sp)+,%d7  // should never get here!!!
	rts

	.balign 4
	.globl _sys_trap1_SuperState
_sys_trap1_SuperState:
	/*
	 * This is how the supervisor stack looks like...
	 * 0(%ssp) SR (WORD)
	 * 2(%ssp) PC (LONG) - contains address of instruction after
	 *                     trap #1 above
	 *
	 * And this is how the user stack looks like
         * 0(%usp) PC (LONG) - instruction after from where the SuperState()
	 *                     was called
	 * 4(%usp) D7 (LONG)
         */
	move.l	%ssp,%d0
	move.l	%a0,-4(%sp)   // save %a0 on -4(SSP)
	move.l	%usp,%a0
	move.l	%a0,%ssp      // load ssp with usp

	move.l	4(%ssp),%d7   // restore d7
	move.l	(%ssp),%a0    // move the return PC to where D7 was
	move.l  %a0,4(%ssp)
	move.l	%d0,%a0       // %a0 = initial ssp
	move.l	-4(%a0),%a0   // restore %a0
	adda.l  #4,%ssp       // now (%ssp) is the return PC
	rts
