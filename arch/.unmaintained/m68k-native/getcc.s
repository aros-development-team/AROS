#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.3  1996/10/24 15:51:30  aros
#    Use the official AROS macros over the __AROS versions.
#
#    Revision 1.2  1996/08/01 17:41:35  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

#*****************************************************************************
#
#   NAME
#
#	AROS_LH0(UWORD, GetCC,
#
#   LOCATION
#	struct ExecBase *, SysBase, 88, Exec)
#
#   FUNCTION
#	Read the contents of the sr in a easy and compatible way.
#
#   INPUTS
#
#   RESULT
#	The contents of sr as a UWORD.
#
#   NOTES
#	This function will most likely be implemented by a few instructions
#	directly in the jumptable.
#
#   EXAMPLE
#
#   BUGS
#
#   SEE ALSO
#	SetSR()
#
#   INTERNALS
#
#   HISTORY
#
#******************************************************************************

	# This function is implemented directly in the jumptable - but it
	# doesn't harm to see what it looks like.
	# 68000 version
	.globl	_Exec_GetCC
_Exec_GetCC:
	movew	sr,d0
	rts

	# 68010 and up
	.globl	_Exec_GetCC_01
_Exec_GetCC_01:
	movew	ccr,d0
	rts

