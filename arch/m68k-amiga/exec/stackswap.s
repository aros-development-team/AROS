/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: stackswap.s 30792 2009-03-07 22:40:04Z neil $
*/

/*****************************************************************************
 
    NAME
 
 	AROS_LH1(void, StackSwap,
 
    SYNOPSIS
 	AROS_LHA(struct StackSwapStruct *, newStack, A0),
 
    LOCATION
 	struct ExecBase *, SysBase, 122, Exec)
 
    FUNCTION
 	This function switches to the new stack given by the parameters in the
 	stackswapstruct structure. The old stack parameters are returned in
 	the same structure so that the stack can be restored later
 
    INPUTS
 	newStack - parameters for the new stack
 
    RESULT
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
    HISTORY
 
******************************************************************************/

	#include "machine.i"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(StackSwap,Exec)
	.type	AROS_SLIB_ENTRY(StackSwap,Exec),@function
AROS_SLIB_ENTRY(StackSwap,Exec):
	/* Preserve returnaddress and fix sp */
	move.l	(sp)+,d0

	/* Get pointer to tc_SPLower in a1 (tc_SPUpper is next) */
	move.l	ThisTask(a6),a1
	lea.l	tc_SPLower(a1),a1

	/* Just to be sure interrupts always find a good stackframe */
	jsr	Disable(a6)

	/* Swap Lower boundaries */
	move.l	(a1),d1
	move.l	(a0),(a1)+
	move.l	d1,(a0)+

	/* Swap higher boundaries */
	move.l	(a1),d1
	move.l	(a0),(a1)
	move.l	d1,(a0)+

	/* Swap stackpointers */
	move.l	sp,d1
	move.l	(a0),sp
	move.l	d1,(a0)

	/* Reenable interrupts. */
	jsr	Enable(a6)

	/* Restore returnaddress and return */
	move.l	d1,-(sp)
	rts

