#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.2  1996/08/01 17:41:30  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

	IDNestCnt   =	0x126

	.globl	_Exec_Disable
_Exec_Disable:
	# increment nesting count and return
	addqb	#1,a6@(IDNestCnt)
	rts

