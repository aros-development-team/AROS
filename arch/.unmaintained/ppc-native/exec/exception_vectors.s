/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

.space 0x100
.space 0x200
.space 0x300
.space 0x400
.space 0x500
.space 0x600
/* handler for program type exceptions */
.space 0x700
 
	push	scr
	mflr	scr	/* save lr, so we can move it to srr0 later */
	push	scr
	mfsrr0	scr
	/* was it called from Supervisor function? */
	cmp	scr,_Supervisor_trp
	beq	_superv_fn
	/* was it called from Superstate function? */
	cmp	scr,_Superstate_trp
	beq	_supers_fn

-----------------------
/* not yet converted to PPC */

 	/* Store trap number */
 pv:	move.l	#8,-(sp)
 	bra	_TrapEntry
 
 	/* And handle the trap */
 _TrapEntry:
 	/* Simple disable */
 	or.w	#0x0700,sr
 
 	/* get some room for destination address */
 	subq.w	#4,sp
 
 	/* calculate destination address without clobbering any registers */
 	move.l	a0,-(sp)
	move.l	4,a0
	move.l	ThisTask(a0),a0
	move.l	tc_TrapCode(a0),4(sp)
	move.l	(sp)+,a0

	/* and jump */
	pop	scr
	mtlr	scr
	pop	scr
	blr

----------------------

_superv_fn:
	/* execute user function */
	ljmp	arg0
_supers_fn:
	/* restore lr */
	pop	scr
	mtlr	scr
	/* restore scr saved in Superstate() */
	pop	scr
	/* fake "supervisor stack" address */
	mr	ret,sp
	/* return from Superstate() subroutine */
	blr
	
