/*
     (C) 1998 AROS - The Amiga Research OS
     $Id$

     Desc: Exec function Dispatch()
     Lang: english
*/

/******************************************************************************

    NAME
	AROS_LH0(void, Switch,

    LOCATION
	struct ExecBase *, SysBase, 9, Exec)

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
#include "supervisor.i"

	.text
	.globl	AROS_SLIB_ENTRY(Switch,Exec)
	.type	AROS_SLIB_ENTRY(Switch,Exec),@function

AROS_SLIB_ENTRY(Switch,Exec):
	pushl	%ecx
	movl	8(%esp),%eax
	movl	ThisTask(%eax),%ecx
	cmpb	$TS_RUN,tc_State(%ecx)
	jne	.skip_resch
	testb	$TF_EXCEPT,tc_Flags(%ecx)
	jne	.skip_resch
	movb	$TS_READY,tc_State(%ecx)
	pushl	%eax
	pushl	%ecx
	call	*Reschedule(%eax)
	addl	$8,%esp
.skip_resch:
	popl	%ecx
	cmpb	$0,supervisor	/* are we in supervisor? */
	jne	.super_ok	/* if yes just call Dispatch */

	popl	%eax		/* Make stack frame similar to */
	pushf			/* the one in supervisor mode. */
	pushl	%cs		/* You have to do iret after calling */
	pushl	%eax		/* Dispatch... */
	
	movl	4,%eax /* 12(%esp),%eax */
	SUPER			/* Enter supervisor mode */
	REG_STORE		/* Create stack frame */
	call	*Dispatch(%eax)	/* Call Dispatch */
	REG_RESTR		/* Restore new stack frame */
	USER			/* Enter user mode */
	iret			/* Return (new task)! */

.super_ok:
	movl	4(%esp),%eax
	call	*Dispatch(%eax)	/* This usually occurs inside interrupts */
	ret