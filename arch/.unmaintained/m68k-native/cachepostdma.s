#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.4  1996/10/24 15:51:29  aros
#    Use the official AROS macros over the __AROS versions.
#
#    Revision 1.3  1996/10/21 21:08:57  aros
#    Changed AROS_LA to AROS_LHA
#
#    Revision 1.2  1996/08/01 17:41:33  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

#******************************************************************************
#
#   NAME
#
#	AROS_LH3(void, CachePostDMA,
#
#   SYNOPSIS
#	AROS_LHA(APTR,    address, A0),
#	AROS_LHA(ULONG *, length,  A1),
#	AROS_LHA(ULONG,   flags,  D0),
#
#   LOCATION
#	struct ExecBase *, SysBase, 128, Exec)
#
#   FUNCTION
#	Do everything necessary to make CPU caches aware that a DMA has
#	happened.
#
#   INPUTS
#	address - Virtual address of memory affected by the DMA
#	*length - Number of bytes affected
#	flags	- DMA_NoModify	  - Indicate that the memory didn't change.
#		  DMA_ReadFromRAM - Indicate that the DMA goes from RAM
#				    to the device. Set this bit in bot calls.
#
#   RESULT
#
#   NOTES
#	DMA must follow a call to CachePreDMA() and must be followed
#	by a call to CachePostDMA().
#
#   EXAMPLE
#
#   BUGS
#
#   SEE ALSO
#	CachePreDMA()
#
#   INTERNALS
#
#   HISTORY
#
#******************************************************************************

	# Simple 68000s have no chaches
	.globl	_Exec_CachePostDMA
_Exec_CachePostDMA:
	rts

