#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.7  1996/10/24 15:51:11  aros
#    Use the official AROS macros over the __AROS versions.
#
#    Revision 1.6  1996/10/23 08:04:25  aros
#    Use generated offsets which makes porting much easier
#
#    Revision 1.5  1996/09/11 16:54:27	digulla
#    Always use __AROS_SLIB_ENTRY() to access shared external symbols, because
#	some systems name an external symbol "x" as "_x" and others as "x".
#	(The problem arises with assembler symbols which might differ)
#
#    Revision 1.4  1996/08/23 16:49:21	digulla
#    With some systems, .align 16 aligns to 64K instead of 16bytes. Therefore
#	I replaced it with .balign which does what we want.
#
#    Revision 1.3  1996/08/13 14:03:19	digulla
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
#	AROS_LH0(void, Forbid,
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

	.include "machine.i"

	.text
	.balign 16
	.globl	_Exec_Forbid
	.type	_Exec_Forbid,@function
_Exec_Forbid:
	pushl %eax
	movl 8(%esp),%eax
	incb TDNestCnt(%eax)
	popl %eax
	ret
