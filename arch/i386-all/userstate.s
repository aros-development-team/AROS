#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.5  1996/09/11 16:54:30  digulla
#    Always use __AROS_SLIB_ENTRY() to access shared external symbols, because
#    	some systems name an external symbol "x" as "_x" and others as "x".
#    	(The problem arises with assembler symbols which might differ)
#
#    Revision 1.4  1996/08/23 16:49:22	digulla
#    With some systems, .align 16 aligns to 64K instead of 16bytes. Therefore
#	I replaced it with .balign which does what we want.
#
#    Revision 1.3  1996/08/13 14:03:21	digulla
#    Added standard headers
#
#    Revision 1.2  1996/08/01 17:41:21	digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

#*****************************************************************************
#
#   NAME
#	__AROS_LH0(void, UserState,
#
#   LOCATION
#	struct ExecBase *, SysBase, 26, Exec)
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
	.balign 16
	.globl	_Exec_UserState
	.type	_Exec_UserState,@function
_Exec_UserState:
	/* Dummy */
	ret

