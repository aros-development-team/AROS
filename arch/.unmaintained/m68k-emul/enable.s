#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.3  1996/12/05 15:30:59  aros
#    Patches by Geert Uytterhoeven integrated
#
#    Revision 1.2  1996/08/01 17:41:31  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

	.include "machine.i"

	.globl	_Exec_Enable
	.type	_Exec_Enable,@function
_Exec_Enable:
	# decrement nesting count and return if there are Disable()s left
	subqb	#1,%a6@(IDNestCnt)
	jpl	end
	jsr	en

	# return if there are no delayed switches pending.
	tstb	%a6@(AttnResched+1)
	jpl	end

	# if TDNestCnt is not -1 taskswitches are still forbidden
	tstb	%a6@(TDNestCnt)
	jpl	end

	# Unset delayed switch bit and do the delayed switch
	bclr	#7,%a6@(AttnResched+1)
	jsr	%a6@(Switch)

	# all done.
end:	rts

	.globl	en
	.type	en,@function
en:
	moveq	#-1,%d0
	movel	%d0,%sp@-
	moveq	#0,%d0
	movel	%d0,%sp@-
	pea	%sp@(4)
	moveq	#-1,%d0
	movel	%d0,%sp@-
	jsr	sigprocmask
	lea	%sp@(16),%sp
	rts
