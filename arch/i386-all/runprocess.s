#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.5  1996/09/11 16:54:28  digulla
#    Always use __AROS_SLIB_ENTRY() to access shared external symbols, because
#    	some systems name an external symbol "x" as "_x" and others as "x".
#    	(The problem arises with assembler symbols which might differ)
#
#    Revision 1.4  1996/08/23 16:49:21	digulla
#    With some systems, .align 16 aligns to 64K instead of 16bytes. Therefore
#	I replaced it with .balign which does what we want.
#
#    Revision 1.3  1996/08/13 14:03:19	digulla
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
	.balign 16
	.globl	_Dos_RunProcess
	.type	_Dos_RunProcess,@function
_Dos_RunProcess:
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
