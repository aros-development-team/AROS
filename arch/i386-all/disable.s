#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.2  1996/08/01 17:41:24  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

	IDNestCnt   =	302

	.text
	.align	16
	.globl	Exec_Disable
	.type	Exec_Disable,@function
Exec_Disable:
	pushl %eax
	movl 8(%esp),%eax
	incb IDNestCnt(%eax)
	popl %eax
	ret
