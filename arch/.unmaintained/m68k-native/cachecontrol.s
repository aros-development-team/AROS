#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.2  1996/08/01 17:41:33  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

#*****************************************************************************
#
#   NAME
#
#	__AROS_LH2(ULONG, CacheControl,
#
#   SYNOPSIS
#	__AROS_LA(ULONG, cacheBits, D0),
#	__AROS_LA(ULONG, cacheMask, D1),
#
#   LOCATION
#	struct ExecBase *, SysBase, 107, Exec)
#
#   FUNCTION
#	Change/read the values in the 68030 cacr register. Only the bits set
#	in the mask parameter are affected.
#
#   INPUTS
#	cacheBits - new bit values.
#	cacheMask - Bits to change.
#
#   RESULT
#	Old contents of cacr register.
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

	# Simple 68000s have no chaches
	.globl	_Exec_CacheControl
_Exec_CacheControl:
	rts

