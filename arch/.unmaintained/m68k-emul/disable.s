#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.3  1996/12/05 15:30:59  aros
#    Patches by Geert Uytterhoeven integrated
#
#    Revision 1.2  1996/08/01 17:41:30  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

	.include "machine.i"

	.globl	_Exec_Disable
	.type	_Exec_Disable,@function
_Exec_Disable:
	# increment nesting count and return
	addqb	#1,%a6@(IDNestCnt)
	rts

	.globl	disable
	.type	disable,@function
disable:
	moveq	#-1,%d0
	movel	%d0,%sp@-
	moveq	#0,%d0
	movel	%d0,%sp@-
	pea	%sp@(4)
	movel	%d0,%sp@-
	jsr	sigprocmask
	lea	%sp@(16),%sp
	rts
