/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang:
*/

/******************************************************************************

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
	.balign 16
	.globl	AROS_SLIB_ENTRY(Switch,Exec)
	.type	AROS_SLIB_ENTRY(Switch,Exec),@function
AROS_SLIB_ENTRY(Switch,Exec):
	/* Make room for Dispatch() address. */
	subl	$4,%esp

	/* Preserve registers */
	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx

	/* Get SysBase */
	movl	24(%esp),%ebx

	/* If current state is TS_RUN and TF_EXCEPT is 0... */
	movl	ThisTask(%ebx),%ecx
	movw	tc_Flags(%ecx),%eax
	andb	$TF_EXCEPT,%al
	cmpw	$TS_RUN*256,%ax
	jne	disp

	/* ...move task to the ready list */
	movb	$TS_READY,tc_State(%ecx)
	leal	Enqueue(%ebx),%edx
	pushl	%ebx
	pushl	%ecx
	leal	TaskReady(%ebx),%eax
	pushl	%eax
	call	*%edx
	addl	$12,%esp

	/* Prepare dispatch */
disp:	leal	Dispatch(%ebx),%eax
	movl	%eax,16(%esp)

	/* restore registers and dispatch */
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%eax
	ret
