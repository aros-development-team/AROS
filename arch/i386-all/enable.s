#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.9  1996/10/23 08:04:25  aros
#    Use generated offsets which makes porting much easier
#
#    Revision 1.8  1996/10/18 07:24:42	aros
#    Just removed my test-code :-)
#
#    Revision 1.7  1996/10/18 01:12:52	aros
#    Added small patch to tell FreeBSD to use _sigprocmask not sigprocmask.
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
#	__AROS_LH0(void, Enable,
#
#   LOCATION
#	struct ExecBase *, SysBase, 21, Exec)
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
#if defined(__FreeBSD__)
#define sigprocmask _sigprocmask
#endif

	.include "machine.i"

	.text
	.balign 16
	.globl	_Exec_Enable
	.type	_Exec_Enable,@function
_Exec_Enable:
	/* Preserve all registers */
	pushl	%edx
	pushl	%eax

	/* Get SysBase */
	movl	12(%esp),%edx

	/* Decrement and test IDNestCnt */
	decb	IDNestCnt(%edx)
	jge	noswch
	call	en

	/* Is there a delayed switch pending and are taskswitches allowed? */
	cmpb	$0,AttnResched(%edx)
	jge	noswch
	cmpb	$0,TDNestCnt(%edx)
	jge	noswch

	/* Clear delayed switch bit and do the delayed switch */
	andb	$127,AttnResched(%edx)
	leal	Switch(%edx),%eax
	pushl	%edx
	call	*%eax
	addl	$4,%esp

	/* Restore registers and return */
noswch: popl	%eax
	popl	%edx
	ret

.globl en
	.type	en,@function
en:
	pushl %eax
	pushl %ecx
	pushl %edx
	pushl $-1
	pushl $0
	leal 4(%esp),%eax
	pushl %eax
	pushl $1
	call sigprocmask
	addl $16,%esp
	popl %edx
	popl %ecx
	popl %eax
	ret
