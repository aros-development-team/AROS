#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.5  1996/11/16 01:31:03  aros
#    Fixed hex from $ to 0x
#
#    Revision 1.4  1996/11/01 02:05:23  aros
#    Motorola syntax (no more MIT)
#
#    Revision 1.3  1996/10/24 15:51:29  aros
#    Use the official AROS macros over the __AROS versions.
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
#	AROS_LH0(void, CacheClearU,
#
#   LOCATION
#	struct ExecBase *, SysBase, 106, Exec)
#
#   FUNCTION
#	Flushes the contents of all CPU chaches in a simple way.
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

	Supervisor  =	-0x1e

	# Simple 68000s have no chaches
	.globl	_Exec_CacheClearU
_Exec_CacheClearU:
	rts

	# Is this the same routine for 20?
	.globl	_Exec_CacheClearU_30
_Exec_CacheClearU_30:
	# Do the real work in supervisor mode
	# Preserve a5 in a1 (faster than stack space)
	move.l	a5,a1
	lea.l	cacheclearusup,a5
	jsr	Supervisor(a6)
	move.l	a1,a5
	rts

cacheclearusup:
	# Set CD and CI bit in cacr
	movec	cacr,d0
	or.w	#0x0808,d0
	movec	d0,cacr
	rte

