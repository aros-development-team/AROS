#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#
#    Desc: Exec function Disable
#    Lang: english

#*****************************************************************************
#
#   NAME
#	__AROS_LH0(void, Disable,
#
#   LOCATION
#	struct ExecBase *, SysBase, 20, Exec)
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

	IDNestCnt   =	302

	.text
	.balign 16
	.globl	_Exec_Disable
	.type	_Exec_Disable,@function
_Exec_Disable:
	call dis
	pushl %eax
	movl 8(%esp),%eax
	incb IDNestCnt(%eax)
	popl %eax
	ret

.globl dis
	.type	dis,@function
dis:
	pushl %eax
	pushl %ecx
	pushl %edx
	pushl $-1
	pushl $0
	leal 4(%esp),%eax
	pushl %eax
	pushl $0
	call sigprocmask
	addl $16,%esp
	popl %edx
	popl %ecx
	popl %eax
	ret
