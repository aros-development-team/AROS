/*
    (C) 1995-96 AROS - The Amiga Replacement OS
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
	/* Preserve scratch registers */
	movem.l	%d0-%d1/%a0-%a1,-(%sp)

	/* Get SysBase */
	move.l	24(%sp),%a0

	/* If not in state TS_RUN the current task is already moved
	   to one of the task lists. */
	move.l	ThisTask(%a0),%a1
	btst	#TB_EXCEPT,tc_Flags(%a1)
	bne	disp
	cmp.b	#TS_RUN,tc_State(%a1)
	bne	disp

	/* ...move task to the ready list */
	move.b	#TS_READY,tc_State(%a1)
	move.l	%a0,-(%sp)
	move.l	%a1,-(%sp)
	pea	TaskReady(%a0)
	jsr	Enqueue(%a0)
	addq.w	#8,%sp
	addq.w	#4,%sp

	/* dispatch */
disp:	movem.l	(%sp)+,%d0-%d1/%a0-%a1
	jmp	AROS_SLIB_ENTRY(Dispatch,Exec)
