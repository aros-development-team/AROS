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
	/* Push all registers */
	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%edi
	pushl	%esi
	pushl	%ebp

	/* Get SysBase */
	movl	32(%esp),%ecx

	/* block all signals */
	call	_disable

	/* Store sp */
	movl	ThisTask(%ecx),%edx
	movl	%esp,tc_SPReg(%edx)

	/* Switch bit set? */
	testb	$TF_SWITCH,tc_Flags(%edx)
	je	.noswch
	movl	tc_Switch(%edx),%eax
	call	*%eax

.noswch:
	/* Store IDNestCnt */
	movb	IDNestCnt(%ecx),%al
	movb	%al,tc_IDNestCnt(%edx)
	movb	$-1,IDNestCnt(%ecx)

	/* Get task from ready list */
	movl	TaskReady(%ecx),%edx
	movl	(%edx),%eax
	movl	%eax,TaskReady(%ecx)
	movl	(%edx),%eax
	leal	TaskReady(%ecx),%ebx
	movl	%ebx,4(%eax)
	movl	%edx,ThisTask(%ecx)

	/* Use as current task */
	movb	$TS_RUN,tc_State(%edx)
	movb	tc_IDNestCnt(%edx),%al
	movb	%al,IDNestCnt(%ecx)

	/* Launch bit set? */
	cmpb	$0,tc_Flags(%edx)
	jge	.nolnch
	movl	tc_Launch(%edx),%eax
	call	*%eax

.nolnch:
	/* Get new sp */
	movl	tc_SPReg(%edx),%eax

	/* Compare agains SPLower */
	cmpl	%eax,tc_SPLower(%edx)
	ja	.alert

	/* Compare against SPUpper */
	cmpl	%eax,tc_SPUpper(%edx)
	ja	.ok

.alert:
	/* Call Alert() */
	pushl	%ecx
	pushl	$(AT_DeadEnd|AN_StackProbe)
	leal	Alert(%ecx),%eax
	call	*%eax
	/* Function does not return */

.called_alert:
	nop

.ok:
	/* Put the SP into the correct register after checking */
	movl	%eax,%esp

	/* Unblock signals if necessary */
	cmpb	$0,tc_IDNestCnt(%edx)
	jge	.noen
	/* If called from the signal handler don't do it. */
	cmpb	$0,AROS_CSYMNAME(supervisor)
	jne	.noen
	call	en

.noen:
	/* Except bit set? */
	testb	$TF_EXCEPT,tc_Flags(%edx)
	je	.noexpt

	/* Raise task exception in Disable()d state */
	pushl	%ecx
	leal	Disable(%ecx),%eax
	call	*%eax
	leal	Exception(%ecx),%eax
	call	*%eax
	movl	(%esp),%ecx
	leal	Enable(%ecx),%eax
	call	*%eax
	addl	$4,%esp

	/* Restore registers and return */
.noexpt:
	popl	%ebp
	popl	%esi
	popl	%edi
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%eax
	ret

