#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.7  1996/10/23 08:04:24  aros
#    Use generated offsets which makes porting much easier
#
#    Revision 1.6  1996/10/10 13:24:47	digulla
#    Make timer work (Fleischer)
#
#    Revision 1.5  1996/09/11 16:54:26	digulla
#    Always use __AROS_SLIB_ENTRY() to access shared external symbols, because
#	some systems name an external symbol "x" as "_x" and others as "x".
#	(The problem arises with assembler symbols which might differ)
#
#    Revision 1.4  1996/08/23 16:49:20	digulla
#    With some systems, .align 16 aligns to 64K instead of 16bytes. Therefore
#	I replaced it with .balign which does what we want.
#
#    Revision 1.3  1996/08/13 14:03:18	digulla
#    Added standard headers
#
#    Revision 1.2  1996/08/01 17:41:25	digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

#*****************************************************************************
#
#   NAME
#	__AROS_LH0(void, Dispatch,
#
#   LOCATION
#	struct ExecBase *, SysBase, 7, Exec)
#
#   FUNCTION
#
#   INPUTS
#
#   RESULT
#
#   NOTES
#
#   EXAMPLE
#
#   BUGS
#
#   SEE ALSO
#
#   INTERNALS
#
#   HISTORY
#
#******************************************************************************

	.include "machine.i"

	.text
	.balign 16
	.globl	_Exec_Dispatch
	.type	_Exec_Dispatch,@function
_Exec_Dispatch:
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
	call	dis

	/* Store sp */
	movl	ThisTask(%ecx),%edx
	movl	%esp,tc_SPReg(%edx)

	/* Switch bit set? */
	testb	$TF_SWITCH,tc_Flags(%edx)
	je	noswch
	movl	tc_Switch(%edx),%eax
	call	*%eax

	/* Store IDNestCnt */
noswch: movb	IDNestCnt(%ecx),%al
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
	jge	nolnch
	movl	tc_Launch(%edx),%eax
	call	*%eax

	/* Get new sp */
nolnch: movl	tc_SPReg(%edx),%esp

	/* Unblock signals if necessary */
	cmpb	$0,tc_IDNestCnt(%edx)
	jge	noen
	call	en

noen:	/* Except bit set? */
	testb	$TF_EXCEPT,tc_Flags(%edx)
	je	noexpt

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
noexpt: popl	%ebp
	popl	%esi
	popl	%edi
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%eax
	ret

