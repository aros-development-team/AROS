#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.4  1996/08/23 16:49:21  digulla
#    With some systems, .align 16 aligns to 64K instead of 16bytes. Therefore
#    	I replaced it with .balign which does what we want.
#
#    Revision 1.3  1996/08/13 14:03:19  digulla
#    Added standard headers
#
#    Revision 1.2  1996/08/01 17:41:25	digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

	TDNestCnt   =	303

#*****************************************************************************
#
#   NAME
#	__AROS_LH0(void, Forbid,
#
#   LOCATION
#	struct ExecBase *, SysBase, 22, Exec)
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
	.text
	.balign	16
	.globl	Exec_Forbid
	.type	Exec_Forbid,@function
Exec_Forbid:
	pushl %eax
	movl 8(%esp),%eax
	incb TDNestCnt(%eax)
	popl %eax
	ret
