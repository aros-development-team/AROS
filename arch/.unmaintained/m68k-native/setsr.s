#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.5  1996/11/01 02:05:24  aros
#    Motorola syntax (no more MIT)
#
#    Revision 1.4  1996/10/24 15:51:31  aros
#    Use the official AROS macros over the __AROS versions.
#
#    Revision 1.3  1996/10/21 21:08:58  aros
#    Changed AROS_LA to AROS_LHA
#
#    Revision 1.2  1996/08/01 17:41:36  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

#*****************************************************************************
#
#   NAME
#
#	AROS_LH2(ULONG, SetSR,
#
#   SYNOPSIS
#	AROS_LHA(ULONG, newSR, D0),
#	AROS_LHA(ULONG, mask,  D1),
#
#   LOCATION
#	struct ExecBase *, SysBase, 24, Exec)
#
#   FUNCTION
#	Read/Modify the CPU status register in an easy way. Only the bits set in
#	the mask parameter will be changed.
#
#   INPUTS
#	newSR - New contents of sr.
#	mask  - Bits to change.
#
#   RESULT
#	Old contents of sr.
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

	Supervisor  =	-0x1e

	.globl	_Exec_SetSR
_Exec_SetSR:
	# Do the real work in supervisor mode
	# Preserve a5 in a0 (faster than stack space)
	move.l	a5,a0
	lea.l	setsrsup,a5
	jsr	Supervisor(a6)
	move.l	a0,a5
	rts

setsrsup:
	# The old value of sr now lies on the top of the stack.
	# d1 = (mask & newSR) | (~mask & SR)
	and.w	d1,d0
	eor.w	#-1,d1
	and.w	(sp),d1
	or.w	d0,d1

	# Get returncode
	clr.l	d0
	move.w	(sp),d0

	# Set new sr value
	move.w	d1,(sp)
	rte

