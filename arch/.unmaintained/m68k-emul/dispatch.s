/*
     (C) 1995-96 AROS - The Amiga Replacement OS
     $Id$

     Desc: Exec function Dispatch()
     Lang: english
*/

/******************************************************************************

    NAME
        AROS_LH0(void, Dispatch,

    LOCATION
        struct ExecBase *, SysBase, 7, Exec)

    FUNCTION

    INPUTS

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
	.balign 16
	.globl	AROS_SLIB_ENTRY(Dispatch,Exec)
	.type	AROS_SLIB_ENTRY(Dispatch,Exec),@function

AROS_SLIB_ENTRY(Dispatch,Exec):
	/* move whole user context to user stack */
	movem.l	%d0-%d7/%a0-%a6,-(%sp)
	move.l	64(%sp),%a4

	jbsr	AROS_CSYMNAME(disable)

	/* get current task and store sp there */
	move.l	ThisTask(%a4),%a2
	move.l	%sp,tc_SPReg(%a2)

	/* call the switch routine if necessary */
	btst	#TB_SWITCH,tc_Flags(%a2)
	jeq	.noswch
	move.l	tc_Switch(%a2),%a5
	jsr	(%a5)

	/* store IDNestCnt */
.noswch:
	move.b	IDNestCnt(%a4),tc_IDNestCnt(%a2)
	move.b	#-1,IDNestCnt(%a4)

	/* get address of ready list */
	lea.l	TaskReady(%a4),%a0
	move.l  (%a0),%a2
	move.l	(%a2),%a1
	move.l	%a1,(%a0)
	move.l	%a0,4.w(%a1)

	move.l	%a2,ThisTask(%a4)
	
	/* and use it as new current task */
	move.b	#TS_RUN,%d0
	move.b	%d0,tc_State(%a2)
	move.b	tc_IDNestCnt(%a2),IDNestCnt(%a4)

	/* call the launch routine if necessary */
	btst	#TB_LAUNCH,tc_Flags(%a2)
	beq	.nolnch
	move.l	tc_Launch(%a2),%a5
	jsr	(%a5)

	/* get stack pointer */
.nolnch: 
	move.l	tc_SPReg(%a2),%sp

	/* Unblock signals if necessary */
	tst.b	tc_IDNestCnt(%a2)
	jpl	.noen
	/* If called from the signal handler don't do it. */
	tst.b	AROS_CSYMNAME(supervisor)
	jne	.noen
	jbsr	en

.noen:
	/* Except bit set? */
	btst	#TB_EXCEPT,tc_Flags(%a2)
	beq	.noexpt

	/* Raise a task exception in Disable()d state. */
	move.l	%a4,-(%sp)
	jsr	Disable(%a4)
	jsr	Exception(%a4)
	jsr	Enable(%a4)
	addq.w	#4,%sp

	/* restore context. */
.noexpt:
	movem.l	(%sp)+,%d0-%d7/%a0-%a6
	rts

