#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.3  1996/10/24 01:38:31  aros
#    Include machine.i
#
#    Revision 1.2  1996/08/01 17:41:36  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

#*****************************************************************************
#
#   NAME */
#
#	__AROS_LH0(void, Permit,
#
#   LOCATION
#	struct ExecBase *, SysBase, 23, Exec)
#
#   FUNCTION
#	This function activates the dispatcher again after a call to Permit().
#
#   INPUTS
#
#   RESULT
#
#   NOTES
#	This function preserves all registers.
#
#   EXAMPLE
#
#   BUGS
#
#   SEE ALSO
#	Forbid(), Disable(), Enable()
#
#   INTERNALS
#
#   HISTORY
#
#******************************************************************************

	.include "machine.i"

	.text
	.balign 16
	.globl	_Exec_Permit
	.type	_Exec_Permit,@function

_Exec_Permit:
	# decrement nesting count and return if there are Forbid()s left
	subqb	#1,a6@(TDNestCnt)
	jpl	end

	# return if there are no delayed switches pending.
	tstb	a6@(AttnResched+1)
	jpl	end

	# if IDNestCnt is not -1 taskswitches are still forbidden
	tstb	a6@(IDNestCnt)
	jpl	end

	# Unset delayed switch bit and do the delayed switch
	bclr	#7,a6@(0x12b)
	jsr    a6@(Switch)

	# all done.
end:	rts

