/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************
 
    NAME
 
 	AROS_LH0(void, Dispatch,
 
    LOCATION
 	struct ExecBase *, SysBase, 10, Exec)
 
    FUNCTION
 	This function switches between the task in SysBase->ThisTask and
 	the first task in the ready list. It must be called from supervisor
 	mode with all registers set to the values of the underlying user
 	context and sp pointing to the normal exception frame (just as if
 	it was a routine in one of the interrupt vectors).
 
 	SysBase->IDNestCnt is moved to and from the task structures and
 	the interrupt control is prepared accordingly. The routines in
 	task->tc_Launch and task->tc_Switch are called if necessary and
 	a task exception for the new task is raised if TF_EXCEPT is set.
 
    INPUTS
 
    RESULT
 
    NOTES
 	Raising an exception for a waiting task reactivates the task
 	completely.
 
 	If the current task (when calling this function) is not part of
 	one of the two task lists it gets lost.
 
 	This function is for internal exec use only.
 
 	This function is processor dependant.
 
    EXAMPLE
 
    BUGS
 	This function currently reads the global variable sysbase instead of
 	*(struct ExecBase **)4. This makes it usable for link libraries.
 
    SEE ALSO
 
    INTERNALS
 	For the task lists the following conditions must always be true as
 	long as SysBase->IDNestCnt>=0. Changes are legal with interrupts
 	Disable()d only:
 
 	* A task is in state TS_WAIT if and only if it is part of the waiting
 	  list.
 
 	* It is in state TS_READY if and only if it is part of the ready list.
 
 	* SysBase->ThisTask always points to a legal task structure.
 
 	* In normal user context SysBase->ThisTask->tc_State is TS_RUN.
 
 	  There are two possible exceptions from this rule:
 
 	  * In supervisor mode when bit 15 of SysBase->AttnResched is set
 	    (dispatch pending bit).
 	    This ends by calling Dispatch() before falling down to user context.
 
 	  * In exec code. This ends by calling Switch() before going back to
 	    user code.
 
 	* The task in SysBase->ThisTask is one of the ready tasks with the
 	  highest priority (round robin). There's an exception from this rule,
 	  too:
 
 	  * If dispatching is disabled and bit 7 of SysBase->AttnResched is set
 	    (switch pending bit) it is allowed to have a running task with a
 	    lower priority as a waiting one. This ends by calling Switch() as
 	    soon as the dispatcher is reactivated.
 
 	* The ready list is sorted by priority.
 
 	* There is always at least one task willing to run - the busy task.
 
    HISTORY
 
******************************************************************************/

	INTENA	    =	0xdff09a
	INTEN	    =	0x4000
	SET	    =	0x8000

	/* Dispatching routine for the 68000.
	   Higher models (with FPU) need a slightly different
	   routine or the additional registers cannot be used!
	*/

	#include "machine.i"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(Dispatch,Exec)
	.type	AROS_SLIB_ENTRY(Dispatch,Exec),@function
AROS_SLIB_ENTRY(Dispatch,Exec):

	/* preserve a5 then move user stack pointer into it */
	move.l	a5,-(sp)
	move.l	usp,a5

	/* move whole user context to user stack */
	move.l	(sp)+,-(a5)
	move.w	(sp)+,-(a5)
	move.l	(sp)+,-(a5)
	movem.l d0-d7/a0-a4/a6,-(a5)

	/* get SysBase */
	move.l	_SysBase,a6

	/* disable interrupts the simple way */
	move.w	#0x2700,sr

	/* get current task and store usp there */
	move.l	ThisTask(a6),a2
	move.l	a5,tc_SPReg(a2)

	/* call the switch routine if necessary */
	btst	#TB_SWITCH,tc_Flags(a2)
	beq	noswch
	move.l	tc_Switch(a2),a5
	jsr	(a5)

	/* store IDNestCnt and reenable interrupt hardware */
noswch: move.b	IDNestCnt(a6),tc_IDNestCnt(a2)
	move.b	#-1,IDNestCnt(a6)
	move.w	#INTEN+SET,INTENA

	/* get address of ready list */
	lea.l	TaskReady(a6),a0

	/* remove first ready task in the list */
	move.l	(a0),a2
	move.l	(a2),a1
	move.l	a1,(a0)
	move.l	a0,4.w(a1)

	/* and use it as new current task */
	move.l	a2,ThisTask(a6)
	move.b	#TS_RUN,d0
	move.b	d0,tc_State(a2)

	/* restore IDNestCnt and disable interrupt hardware if necessary */
	move.b	tc_IDNestCnt(a2),IDNestCnt(a6)
	bpl	nodis
	move.w	#INTEN,INTENA

	/* call the launch routine if necessary */
nodis:	btst	#TB_LAUNCH,tc_Flags(a2)
	beq	nolnch
	move.l	tc_Launch(a2),a5
	jsr	(a5)

	/* get user stack pointer */
nolnch: move.l	tc_SPReg(a2),a5

	/* test task exception bit */
	btst	#TB_EXCEPT,tc_Flags(a2)
	bne	exc

	/* not set. read complete user context */
	movem.l (a5)+,d0-d7/a0-a4/a6
	move.l	(a5)+,-(sp)
	move.w	(a5)+,-(sp)
	move.l	(a5)+,-(sp)

	/* restore usp and a5 and return */
	move.l	a5,usp
	move.l	(sp)+,a5
	rte

	/* Raise a task exception.
	   The user stack looks like: a5, ccr, pc, a6, a4-a0, d7-d0.
	   Change that to	     pc, ccr, a5, a6, a4-a0, d7-d0
	   so that it's easier to restore the context.
	*/
exc:	move.l	14*4(a5),d0
	move.l	14*4+6(a5),14*4(a5)
	move.l	d0,14*4+6(a5)

	/* do a Disable() to fall down to user context atomically */
	jsr	Disable(a6)

	/* prepare going to user mode */
	move.l	a5,usp
	move.l	#usrexc,-(sp)
	clr.w	-(sp)
	rte

	/* handle exception */
usrexc: jsr	Exception(a6)
	jsr	Enable(a6)

	/* restore context */
	movem.l (sp)+,d0-d7/a0-a4/a6
	move.l	(sp)+,a5
	move.w	(sp)+,ccr
	rts


	.globl	AROS_SLIB_ENTRY(DispatchSup,Exec)
	.type	AROS_SLIB_ENTRY(DispatchSup,Exec),@function
AROS_SLIB_ENTRY(DispatchSup,Exec):
	move.l	AROS_SLIB_ENTRY(Dispatch,Exec),a5
	jsr	Supervisor(a5)
	rte

