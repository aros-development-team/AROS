/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Exec function Switch
    Lang: english
*/

/******************************************************************************

    NAME

        AROS_LH0(void, Switch,

    LOCATION
        struct ExecBase *, SysBase, 9, Exec)

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
	.balign 16
	.globl	AROS_SLIB_ENTRY(Switch,Exec)
	.type	AROS_SLIB_ENTRY(Switch,Exec),@function
AROS_SLIB_ENTRY(Switch,Exec):
	/* Check to see if we are called from supervisor */
	tst.b	AROS_CSYMNAME(supervisor)
	jbeq	.nosup

	/* Called from supervisor mode - set the delayed dispatch flag
           and return */
#if !UseRegisterArgs
	move.l	%a6,-(%sp)
	move.l	8(%sp),%a6
#endif
	bset	#7,AttnResched(%a6)
#if !UseRegisterArgs
	move.l	(%sp)+,%a6
#endif
	rts

	/* Called from user mode */
.nosup:
	/* Preserve scratch registers */
	movem.l	%d0-%d1/%a0-%a1,-(%sp)

	/* Always disable interrupts when testing task lists */
	jbsr	AROS_CSYMNAME(os_disable)
#if !UseRegisterArgs
	/* Get SysBase */
	move.l	20(%sp),%a0

	/* If not in state TS_RUN the current task is already moved
	   to one of the task lists. */
	move.l	ThisTask(%a0),%a1
#else
	move.l	ThisTask(%a6),%a1
#endif
	cmpi.b	#TS_RUN,tc_State(%a1)
	jbne	.disp

	/* If TB_EXCEPT is not set... */
	btst	#TB_EXCEPT,tc_Flags(%a1)
	jbne	.disp

	/* ...move task to the ready list */
	move.b	#TS_READY,tc_State(%a1)
#if !UseRegisterArgs
	move.l	%a0,-(%sp)
	move.l	%a1,-(%sp)
	pea	TaskReady(%a0)
	jsr	Enqueue(%a0)
	addq.w	#8,%sp
	addq.w	#4,%sp
#else
	lea.l	TaskReady(%a6),%a0
	jsr	Enqueue(%a6)
#endif

	/* dispatch */
.disp:	movem.l	(%sp)+,%d0-%d1/%a0-%a1
#if !UseRegisterArgs
	jmp	AROS_SLIB_ENTRY(Dispatch,Exec)
#else
	jmp	Dispatch(%a6)
#endif
