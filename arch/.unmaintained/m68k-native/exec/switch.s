/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************
 
    NAME
 
 	AROS_LH0(void, Switch,
 
    LOCATION
 	struct ExecBase *, SysBase, 6, Exec)
 
    FUNCTION
 	Tries to switch to the first task in the ready list. This
 	function works almost like Dispatch() with the slight difference
 	that it may be called at any time and as often as you want and
 	that it does not lose the current task if it is of type TS_RUN.
 
    INPUTS
 
    RESULT
 
    NOTES
 	This function is CPU dependant.
 
 	This function is for internal use by exec only.
 
 	This function preserves all registers.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 	Dispatch()
 
    INTERNALS
 
    HISTORY
 
******************************************************************************/

	#include "machine.i"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(Switch,Exec)
	.type	AROS_SLIB_ENTRY(Switch,Exec),@function
AROS_SLIB_ENTRY(Switch,Exec):
	/* call switch in supervisor mode
	   this is necessary to determine if the current context is user or
	   supervisor mode */
	move.l	a5,-(sp)
	move.l	#switch,a5
	jsr	Supervisor(a6)
	move.l	(sp)+,a5
	rts

switch:
	/* test if called from supervisor mode
	   (supervisor bit is bit 8+5 of sr when calling Switch() ) */
	btst	#5,(sp)
	beq	nosup

	/* called from supervisor mode (grrrr)
	   since I can only Dispatch() when falling down to user mode I
	   must do it later - set the delayed dispatch flag and return */
	bset	#7,AttnResched(a6)
end:	rte

	/* Called from user mode
	   Always disable interrupts when testing task lists */
nosup:	move.w	#0x2700,sr

	/* Preserve scratch registers */
	movem.l	d0/d1/a0/a1,-(sp)

	/* If not in state TS_RUN the current task is part of one of the
	   task lists. */
	move.l	ThisTask(a6),a1
	cmp.b	#TS_RUN,tc_State(a1)
	bne	disp

	/* If TB_EXCEPT is not set... */
	btst	#TB_EXCEPT,tc_Flags(a1)
	bne	disp

	/* ...Move task to the ready list */
	move.b	#TS_READY,tc_State(a1)
	lea.l	TaskReady(a6),a0
	jsr	Enqueue(a6)

	/* dispatch */
disp:	movem.l	(sp)+,d0/d1/a0/a1
	jmp	Dispatch(a6)
