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
#	__AROS_LH3(void, CacheClearE,
#
#   SYNOPSIS
#	__AROS_LA(APTR,  address, A0),
#	__AROS_LA(ULONG, length,  D0),
#	__AROS_LA(ULONG, caches,  D1),
#
#   LOCATION
#	struct ExecBase *, SysBase, 107, Exec)
#
#   FUNCTION
#	Flushes the contents of the CPU chaches for a given area of memory
#	and a given cache.
#
#   INPUTS
#	address - First byte in memory
#	length	- number of bytes (~0 means all addresses).
#	caches	- CPU caches affected
#		    CACRF_ClearI - Instruction cache
#		    CACRF_ClearD - Data cache
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

	# Simple 68000s have no chaches
	.globl	_Exec_CacheClearE
_Exec_CacheClearE:
	rts

