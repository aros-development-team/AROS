#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.3  1996/12/05 15:31:00  aros
#    Patches by Geert Uytterhoeven integrated
#
#    Revision 1.2  1996/08/01 17:41:31  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

	.include "machine.i"

	.globl	_Exec_Permit
	.type	_Exec_Permit,@function
_Exec_Permit:
	# decrement nesting count and return if there are Forbid()s left
	subqb	#1,%a6@(TDNestCnt)
	jpl	end

	# return if there are no delayed switches pending.
	tstb	%a6@(AttnResched+1)
	jpl	end

	# if IDNestCnt is not -1 taskswitches are still forbidden
	tstb	%a6@(IDNestCnt)
	jpl	end

	# Unset delayed switch bit and do the delayed switch
	bclr	#7,%a6@(AttnResched+1)
	jsr	%a6@(Switch)

	# all done.
end:	rts

