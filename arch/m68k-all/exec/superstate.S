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

	#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(SuperState,Exec,25)
	.type	AROS_SLIB_ENTRY(SuperState,Exec,25),@function
AROS_SLIB_ENTRY(SuperState,Exec,25):
	/* Goto supervisor mode. Preserve a5 in d0 (faster than stack space) */
	move.l	%a5,%d0
	lea.l	super,%a5

	/* Do not change user stack - use jmp */
	jmp	Supervisor(%a6)
super:
	/* Restore a5 */
	move.l	%d0,%a5

	/* Check if called from supervisor mode */
	btst	#5,(%sp)
	beq	fromuser

	/* Called from supervisor mode. Just return NULL. */
	moveq.l	#0,%d0
	rte

fromuser:
	/* Called from user mode. Restore sp and return supervisor sp. */
	move.l	%sp,%d0
	move.l	%usp,%sp

	/* usp already points to the returnaddress for the SuperState() call. */
	rts

