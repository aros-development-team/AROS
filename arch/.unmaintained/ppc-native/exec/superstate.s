/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************
 
    NAME
 
 	AROS_LH0(APTR, SuperState,
 
    SYNOPSIS
 
    LOCATION
 	struct ExecBase *, SysBase, 25, Exec)
 
    FUNCTION
 	Enter supervisor mode (like Supervisor()), but return on the normal
 	user stack, so that user stack variables are still there. A call
 	to Userstate() will end this mode.
 
    INPUTS
 
    RESULT
 	Old supervisor stack. NULL if called from supervisor mode.
 
    NOTES
 	The user stack must be big enough to hold all possible nestings
 	of interrupts.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 	UserState(), Supervisor()
 
    INTERNALS
 
    HISTORY
 
******************************************************************************/

	#include "machine.i"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(SuperState,Exec)
	.type	AROS_SLIB_ENTRY(SuperState,Exec),@function
AROS_SLIB_ENTRY(SuperState,Exec):
	push	scr
	mflr	scr
	push	scr
	/* cause a trap */
.global _Superstate_trp:
	mfmsr	r0
	/* no trap? We are in usermode, so restore lr */
	pop	scr
	mtlr	scr
	pop	scr
	/* return 0, because we are in supervisor mode */
	li	ret,0
	/* return from this subroutine */
	blr
