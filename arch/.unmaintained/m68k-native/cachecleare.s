#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.4  1996/10/24 15:51:29  aros
#    Use the official AROS macros over the __AROS versions.
#
#    Revision 1.3  1996/10/21 21:08:56  aros
#    Changed AROS_LA to AROS_LHA
#
#    Revision 1.2  1996/08/01 17:41:33  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

#*****************************************************************************
#
#   NAME
#
#	AROS_LH3(void, CacheClearE,
#
#   SYNOPSIS
#	AROS_LHA(APTR,  address, A0),
#	AROS_LHA(ULONG, length,  D0),
#	AROS_LHA(ULONG, caches,  D1),
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

