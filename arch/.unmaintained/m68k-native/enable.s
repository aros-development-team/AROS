#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.5  1996/11/01 02:05:24  aros
#    Motorola syntax (no more MIT)
#
#    Revision 1.4  1996/10/24 15:51:30  aros
#    Use the official AROS macros over the __AROS versions.
#
#    Revision 1.3  1996/10/24 01:38:31  aros
#    Include machine.i
#
#    Revision 1.2  1996/08/01 17:41:35  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

#*****************************************************************************
#
#   NAME
#
#	AROS_LH0(void, Enable,
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

	INTENA	    =	0xdff09a
	INTEN	    =	0x4000
	SET	    =	0x8000

	.include "machine.i"

	.text
	.balign 16
	.globl	_Exec_Enable
	.type	_Exec_Enable,@function

_Exec_Enable:
	# decrement nesting count and return if there are Disable()s left
	subq.b	#1,IDNestCnt(a6)
	bpl	end

	# reenable interrupts
	move.w	#INTEN+SET,INTENA

	# return if there are no delayed switches pending.
	tst.b	AttnResched+1(a6)
	bpl	end

	# if TDNestCnt is not -1 taskswitches are still forbidden
	tst.b	TDNestCnt(a6)
	bpl	end

	# Unset delayed switch bit and do the delayed switch
	bclr	#7,$12b(a6)
	jsr	Switch(a6)

	# all done.
end:	rts

