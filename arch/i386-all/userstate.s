#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.3  1996/08/13 14:03:21  digulla
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
	.align	16
	.globl	Exec_UserState
	.type	Exec_UserState,@function
Exec_UserState:
	/* Dummy */
	ret

