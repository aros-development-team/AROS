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
#    Revision 1.2  1996/08/01 17:41:12	digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

#*****************************************************************************
#
#   NAME
#	__AROS_LH0(void, GetCC,
#
#   LOCATION
#	struct ExecBase *, SysBase, 88, Exec)
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
	.globl	Exec_GetCC
	.type	Exec_GetCC,@function
Exec_GetCC:
	/* Dummy */
	ret

