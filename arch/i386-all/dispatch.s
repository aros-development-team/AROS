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
	/* Push all registers */
	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%edi
	pushl	%esi
	pushl	%ebp

	/* block all signals */
	call	AROS_CSYMNAME(os_disable)

	/* Get SysBase again */
	movl	32(%esp),%ecx

	/* Store sp */
	movl	ThisTask(%ecx),%edx
	movl	%esp,tc_SPReg(%edx)

	/* Switch bit set? */
	testb	$TF_SWITCH,tc_Flags(%edx)
	je	1f
	movl	tc_Switch(%edx),%eax
	call	*%eax

1:
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
	jge	2f
	movl	tc_Launch(%edx),%eax
	call	*%eax

2:
	/* Get new sp */
	movl	tc_SPReg(%edx),%eax

	/* Compare agains SPLower */
	cmpl	%eax,tc_SPLower(%edx)
	ja	3f

	/* Compare against SPUpper */
	cmpl	%eax,tc_SPUpper(%edx)
	ja	4f

3:
	/* Call Alert() */
	pushl	%ecx
	pushl	$(AT_DeadEnd|AN_StackProbe)
	leal	Alert(%ecx),%eax
	call	*%eax
	/* Function does not return */

	nop

4:
	/* Put the SP into the correct register after checking */
	movl	%eax,%esp

	/* Unblock signals if necessary */
	cmpb	$0,tc_IDNestCnt(%edx)
	jge	5f

	/* If called from the signal handler don't do it. */
	cmpl	$0,AROS_CSYMNAME(supervisor)
	jne	5f
	call	AROS_CSYMNAME(os_enable)

5:
	/* Except bit set? */
	testb	$TF_EXCEPT,tc_Flags(%edx)
	je	6f

	/* Raise task exception in Disable()d state */
	pushl	%ecx

	/* leal    Disable(%ecx),%eax
	call	*%eax */
	leal	Exception(%ecx),%eax
	call	*%eax
	/* movl    (%esp),%ecx
	leal	Enable(%ecx),%eax */
	call	*%eax
	addl	$4,%esp

	/* Restore registers and return */
6:
	popl	%ebp
	popl	%esi
	popl	%edi
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%eax
	ret

