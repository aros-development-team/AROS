#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.2  1996/08/01 17:41:31  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

	TDNestCnt   =	0x127

	.globl	_Exec_Forbid
_Exec_Forbid:
	# increment nesting count and return
	# this seems to be a very unspectacular call and a good job for a C
	# routine - but nothing in C can guarantee to preserve all registers
	# and to increment atomically - so better use this simple assembly
	# routine
	addqb	#1,a6@(TDNestCnt)
	rts


