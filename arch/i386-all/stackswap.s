#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.4  1996/08/23 16:49:22  digulla
#    With some systems, .align 16 aligns to 64K instead of 16bytes. Therefore
#    	I replaced it with .balign which does what we want.
#
#    Revision 1.3  1996/08/13 14:03:20  digulla
#    Added standard headers
#
#    Revision 1.2  1996/08/01 17:41:20	digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

#*****************************************************************************
#
#   NAME
#	__AROS_LH0(void, StackSwap,
#
#   LOCATION
#	struct ExecBase *, SysBase, 122, Exec)
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
	Disable 	= -100
	Enable		= -105
	tc_SPLower	= 60
	ThisTask	= 284

	.text
	.balign	16
	.globl	Exec_StackSwap
	.type	Exec_StackSwap,@function
Exec_StackSwap:
	movl 4(%esp),%edx
	movl 8(%edx),%ecx
	popl %eax
	movl %eax,-12(%ecx)
	popl %eax
	movl %eax,-8(%ecx)
	movl (%esp),%eax
	movl %eax,-4(%ecx)
	addl $-12,%ecx
	leal Disable(%eax),%eax
	call *%eax
	popl %eax
	movl %esp,8(%edx)
	movl %ecx,%esp
	movl ThisTask(%eax),%ecx
	leal tc_SPLower(%ecx),%ecx
	push %ebx
	movl (%edx),%eax
	movl (%ecx),%ebx
	movl %eax,(%ecx)
	movl %ebx,(%edx)
	movl 4(%edx),%eax
	movl 4(%ecx),%ebx
	movl %eax,4(%ecx)
	movl %ebx,4(%edx)
	popl %ebx
	movl 8(%esp),%eax
	pushl %eax
	leal Enable(%eax),%eax
	call *%eax
	addl $4,%esp
	ret
