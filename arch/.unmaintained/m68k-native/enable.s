#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.2  1996/08/01 17:41:35  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

#*****************************************************************************
#
#   NAME
#
#	__AROS_LH0(void, Enable,
#
#   LOCATION
#	struct ExecBase *, SysBase, 21, Exec)
#
#   FUNCTION
#	This function reenables the delivery of interrupts after a call to
#	Disable().
#
#   INPUTS
#
#   RESULT
#
#   NOTES
#	This function preserves all registers.
#
#	This function may be used from interrupts.
#
#   EXAMPLE
#
#   BUGS
#
#   SEE ALSO
#	Forbid(), Permit(), Disable()
#
#   INTERNALS
#
#   HISTORY
#
#******************************************************************************

	Switch	    =	-0x24
	IDNestCnt   =	0x126
	TDNestCnt   =	0x127
	AttnResched =	0x12a
	INTENA	    =	0xdff09a
	INTEN	    =	0x4000
	SET	    =	0x8000

	.globl	_Exec_Enable
_Exec_Enable:
	# decrement nesting count and return if there are Disable()s left
	subqb	#1,a6@(IDNestCnt)
	jpl	end

	# reenable interrupts
	movew	#INTEN+SET,INTENA

	# return if there are no delayed switches pending.
	tstb	a6@(AttnResched+1)
	jpl	end

	# if TDNestCnt is not -1 taskswitches are still forbidden
	tstb	a6@(TDNestCnt)
	jpl	end

	# Unset delayed switch bit and do the delayed switch
	bclr	#7,a6@(0x12b)
	jsr	a6@(Switch)

	# all done.
end:	rts

