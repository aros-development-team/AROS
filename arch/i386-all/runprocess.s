#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.3  1996/08/13 14:03:19  digulla
#    Added standard headers
#
#    Revision 1.2  1996/08/01 17:40:57	digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

	dl_SysBase	= 112
	StackSwap	= -610

	.text
	.align	16
	.globl	RunProcess
	.type	RunProcess,@function
RunProcess:
	pushl %edi
	pushl %esi
	pushl %ebx
	pushl %ebp
	movl 24(%esp),%ebx
	movl 36(%esp),%edi
	movl 4(%ebx),%eax
	addl $-4,%eax
	movl %ebx,(%eax)
	addl $-4,%eax
	movl 40(%esp),%edx
	movl dl_SysBase(%edx),%edx
	movl %edx,(%eax)
	movl %eax,8(%ebx)
	pushl %edx
	pushl %ebx
	leal StackSwap(%edx),%edx
	call *%edx
	addl $8,%esp
	call *%edi
	movl %eax,%esi
	popl %edx
	popl %ebx
	pushl %edx
	pushl %ebx
	leal StackSwap(%edx),%edx
	call *%edx
	addl $8,%esp
	movl %esi,%eax
	popl %ebp
	popl %ebx
	popl %esi
	popl %edi
	ret
