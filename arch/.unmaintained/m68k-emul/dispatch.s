/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Exec function Dispatch()
    Lang: english
*/

/******************************************************************************

    NAME
        AROS_LH0(void, Dispatch,

    LOCATION
        struct ExecBase *, SysBase, 10, Exec)

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
#if !UseRegisterArgs
	move.l	64(%sp),%a6
#endif

	jbsr	AROS_CSYMNAME(os_disable)

	/* get current task and store sp there */
	move.l	ThisTask(%a6),%a2
	move.l	%sp,tc_SPReg(%a2)

	/* call the switch routine if necessary */
	btst	#TB_SWITCH,tc_Flags(%a2)
	jbeq	.noswch
	move.l	tc_Switch(%a2),%a5
	jsr	(%a5)

	/* store IDNestCnt */
.noswch:
	move.b	IDNestCnt(%a6),tc_IDNestCnt(%a2)
	move.b	#-1,IDNestCnt(%a6)

	/* get task from ready list */
	move.l	TaskReady(%a6),%a2
	move.l  (%a2),%a0
	move.l	%a0,TaskReady(%a6)
	lea.l	TaskReady(%a6),%a1
	move.l	%a1,4.w(%a0)
	
	/* and use it as new current task */
	move.l  %a2,ThisTask(%a6)
	move.b	#TS_RUN,tc_State(%a2)
	move.b	tc_IDNestCnt(%a2),IDNestCnt(%a6)

	/* call the launch routine if necessary */
	btst	#TB_LAUNCH,tc_Flags(%a2)
	jbeq	.nolnch
	move.l	tc_Launch(%a2),%a5
	jsr	(%a5)

	/* get stack pointer */
.nolnch: 
	move.l	tc_SPReg(%a2),%d0

	/* Compare against SPLower */
	cmp.l	tc_SPLower(%a2),%d0
	jbls	.alert
	
	/* Compare against SPUpper */
	cmp.l	tc_SPUpper(%a2),%d0
	jbcc	.alert

	/* Put the SP into the correct register after checking */
	move.l	%d0,%sp
	/* Unblock signals if necessary */
	tst.b	tc_IDNestCnt(%a2)
	jbpl	.noen
	/* If called from the signal handler don't do it. */
	tst.b	AROS_CSYMNAME(supervisor)
	jbne	.noen
	jbsr	AROS_CSYMNAME(os_enable)

.noen:
	/* Except bit set? */
	btst	#TB_EXCEPT,tc_Flags(%a2)
	jbeq	.noexpt

	/* Raise a task exception in Disable()d state. */
#if !UseRegisterArgs
	move.l	%a6,-(%sp)
#endif
/*	jsr	Disable(%a6) */
	jsr	Exception(%a6)
/*	jsr	Enable(%a6) */
#if !UseRegisterArgs
	addq.w	#4,%sp
#endif

	/* restore context. */
.noexpt:
	movem.l	(%sp)+,%d0-%d7/%a0-%a6
	rts

.alert:
        /* Call Alert() */
#if !UseRegisterArgs
        move.l  %a6,-(%sp)
#endif
        move.l  #AT_DeadEnd,%d7
        or.l    #AN_StackProbe,%d7
#if !UseRegisterArgs
        move.l  %d0,-(%sp)
#endif
        jsr     Alert(%a6)
        /* Function does not return */
