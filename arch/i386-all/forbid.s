#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.2  1996/08/01 17:41:25  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

	TDNestCnt   =	303

	.text
	.align	16
	.globl	Exec_Forbid
	.type	Exec_Forbid,@function
Exec_Forbid:
	pushl %eax
	movl 8(%esp),%eax
	incb TDNestCnt(%eax)
	popl %eax
	ret
