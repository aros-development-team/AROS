#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.3  1996/08/13 14:03:20  digulla
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
#	__AROS_LH0(void, SuperState,
#
#   LOCATION
#	struct ExecBase *, SysBase, 25, Exec)
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
	.globl	Exec_SuperState
	.type	Exec_SuperState,@function
Exec_SuperState:
	/* Dummy */
	ret

