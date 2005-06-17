/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: dispatch.s 12742 2001-12-08 18:32:01Z chodorowski $

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
	.balign 32  /* twice align of i386??? */
	.globl	AROS_SLIB_ENTRY(Dispatch,Exec)
	.type	AROS_SLIB_ENTRY(Dispatch,Exec),@function

AROS_SLIB_ENTRY(Dispatch,Exec):
	/* Push all registers */
	push	%rax
	push	%rbx
	push	%rcx
	push	%rdx
	push	%rdi
	push	%rsi
	push	%rbp
	push	%r8
	push	%r9
	push	%r10
	push	%r11
	push	%r12
	push	%r13
	push    %r14
	push    %r15
	
	/* block all signals */
	call	AROS_CSYMNAME(os_disable)

	/* Get SysBase again */
	movl	64+56(%rsp),%rcx

	/* Store sp */
	mov	ThisTask(%rcx),%rdx
	mov	%rsp,tc_SPReg(%rdx)

	/* Switch bit set? */
	testb	$TF_SWITCH,tc_Flags(%rdx)
	je	1f
	mov	tc_Switch(%rdx),%rax
	call	*%rax

1:
	/* Store IDNestCnt */
	movb	IDNestCnt(%rcx),%al
	movb	%al,tc_IDNestCnt(%rdx)
	movb	$-1,IDNestCnt(%rcx)

	/* Get task from ready list */
	mov	TaskReady(%rcx),%rdx
	mov	(%rdx),%rax
	mov	%rax,TaskReady(%rcx)
	mov	(%rdx),%rax
	lea	TaskReady(%rcx),%rbx
	mov	%rbx,8(%rax)
	mov	%rdx,ThisTask(%rcx)

	/* Use as current task */
	movb	$TS_RUN,tc_State(%rdx)
	movb	tc_IDNestCnt(%rdx),%al
	movb	%al,IDNestCnt(%rcx)

	/* Launch bit set? */
	cmpb	$0,tc_Flags(%rdx)
	jge	2f
	mov	tc_Launch(%rdx),%rax
	call	*%rax

2:
	/* Get new sp */
	mov	tc_SPReg(%rdx),%rax

	/* Compare agains SPLower */
	cmp	%rax,tc_SPLower(%rdx)
	ja	3f

	/* Compare against SPUpper */
	cmp	%rax,tc_SPUpper(%rdx)
	ja	4f

3:
	/* Call Alert() */
	push	%rcx
	push	$(AT_DeadEnd|AN_StackProbe)
	lea	Alert(%rcx),%rax
	call	*%rax
	/* Function does not return */

	nop

4:
	/* Put the SP into the correct register after checking */
	mov	%rax,%rsp

	/* Unblock signals if necessary */
	cmpb	$0,tc_IDNestCnt(%rdx)
	jge	5f

	/* If called from the signal handler don't do it. */
	cmp	$0,AROS_CSYMNAME(supervisor)
	jne	5f
	call	AROS_CSYMNAME(os_enable)

5:
	/* Except bit set? */
	testb	$TF_EXCEPT,tc_Flags(%rdx)
	je	6f

	/* Raise task exception in Disable()d state */
	push	%rcx

	/* leal    Disable(%rcx),%rax
	call	*%rax */
	lea	Exception(%rcx),%rax
	call	*%rax
	/* mov    (%rsp),%rcx
	lea	Enable(%rcx),%rax */
	call	*%rax
	add	$8,%rsp

	/* Restore registers and return */
6:
	pop	%r15
	pop     %r14
	pop     %r13
	pop     %r12
	pop     %r11
	pop     %r10
	pop     %r9
	pop     %r8
	pop	%rbp
	pop	%rsi
	pop	%rdi
	pop	%rdx
	pop	%rcx
	pop	%rbx
	pop	%rax
	ret

